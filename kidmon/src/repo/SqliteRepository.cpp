#include <kidmon/repo/SqliteRepository.h>
#include <core/utils/File.h>
#include <core/utils/FmtExt.h>

#include "sqlite/SqliteDatabase.h"
#include "sqlite/SqliteStatement.h"
#include "sqlite/SqliteTransaction.h"

#include <spdlog/spdlog.h>

#include <mutex>
#include <span>
#include <stdexcept>
#include <unordered_map>

namespace km {

namespace {

std::int64_t toMillis(const TimePoint tp)
{
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    return duration_cast<milliseconds>(tp.time_since_epoch()).count();
}

TimePoint fromMillis(const std::int64_t ms)
{
    return TimePoint(std::chrono::milliseconds(ms));
}

constexpr std::string_view SCHEMA_SQL = R"sql(
CREATE TABLE IF NOT EXISTS users (
    id       INTEGER PRIMARY KEY,
    username TEXT NOT NULL UNIQUE
);

CREATE TABLE IF NOT EXISTS processes (
    id     INTEGER PRIMARY KEY,
    path   TEXT NOT NULL,
    sha256 TEXT NOT NULL,
    UNIQUE(path, sha256)
);

CREATE TABLE IF NOT EXISTS entries (
    id            INTEGER PRIMARY KEY,
    user_id       INTEGER NOT NULL REFERENCES users(id),
    process_id    INTEGER NOT NULL REFERENCES processes(id),
    window_title  TEXT NOT NULL,
    rect_x        INTEGER NOT NULL,
    rect_y        INTEGER NOT NULL,
    rect_w        INTEGER NOT NULL,
    rect_h        INTEGER NOT NULL,
    image_name    TEXT NOT NULL,
    image_bytes   BLOB NOT NULL,
    image_encoded INTEGER NOT NULL,
    captured_at   INTEGER NOT NULL,
    duration_ms   INTEGER NOT NULL
);
)sql";

// Keeps the SELECT column order and the read-back order from drifting apart.
void hydrateEntry(const sqlite::Statement& row,
                  const std::string& username,
                  Entry& entry)
{
    entry.username = username;
    entry.timestamp.capture = fromMillis(row.columnInt64(0));
    entry.timestamp.duration = std::chrono::milliseconds(row.columnInt64(1));
    entry.windowInfo.title = row.columnText(2);
    entry.windowInfo.placement =
        Rect(Point(static_cast<int>(row.columnInt64(3)),
                   static_cast<int>(row.columnInt64(4))),
             Dimensions(static_cast<uint32_t>(row.columnInt64(5)),
                        static_cast<uint32_t>(row.columnInt64(6))));
    entry.windowInfo.image.name = row.columnText(7);
    entry.windowInfo.image.bytes = row.columnBlob(8);
    entry.windowInfo.image.encoded = row.columnInt64(9) != 0;
    entry.processInfo.processPath = row.columnText(10);
    entry.processInfo.sha256 = row.columnText(11);
}

} // namespace

class SqliteRepository::Impl
{
    fs::path dbFile_;
    sqlite::Database db_;

    // Prepared once, reset() and rebound on every call. Query statements are
    // `mutable` since queryUsers()/queryEntries() are const on IRepository.
    sqlite::Statement upsertUser_;
    sqlite::Statement upsertProcess_;
    sqlite::Statement insertEntry_;
    mutable sqlite::Statement selectUsers_;
    mutable sqlite::Statement selectEntries_;

    // Guards the connection: a query can run concurrently with a write.
    mutable std::mutex mtx_;

    // Users and processes rarely change, so their ids are resolved once.
    std::unordered_map<std::string, std::int64_t> userIds_;
    std::unordered_map<std::string, std::int64_t> processIds_;

    std::int64_t userId(const std::string& username)
    {
        if (const auto it = userIds_.find(username); it != userIds_.end())
        {
            return it->second;
        }

        upsertUser_.reset();
        upsertUser_.bindText(1, username);
        if (!upsertUser_.step())
        {
            throw std::runtime_error("Failed to resolve user id");
        }
        const auto id = upsertUser_.columnInt64(0);
        // A RETURNING statement stays "in progress" after one step(); sqlite
        // refuses to COMMIT until it's reset.
        upsertUser_.reset();

        userIds_.emplace(username, id);
        return id;
    }

    std::int64_t processId(const ProcessInfo& info)
    {
        const auto path = core::file::path2s(info.processPath);
        auto key = path + '\0' + info.sha256;
        if (const auto it = processIds_.find(key); it != processIds_.end())
        {
            return it->second;
        }

        upsertProcess_.reset();
        upsertProcess_.bindText(1, path);
        upsertProcess_.bindText(2, info.sha256);
        if (!upsertProcess_.step())
        {
            throw std::runtime_error("Failed to resolve process id");
        }
        const auto id = upsertProcess_.columnInt64(0);
        upsertProcess_.reset();

        processIds_.emplace(std::move(key), id);
        return id;
    }

    // Caller holds mtx_ and an open transaction.
    void insert(const Entry& entry)
    {
        const auto& placement = entry.windowInfo.placement;

        insertEntry_.reset();
        insertEntry_.bindInt64(1, userId(entry.username))
            .bindInt64(2, processId(entry.processInfo))
            .bindText(3, entry.windowInfo.title)
            .bindInt64(4, placement.leftTop().x())
            .bindInt64(5, placement.leftTop().y())
            .bindInt64(6, placement.width())
            .bindInt64(7, placement.height())
            .bindText(8, entry.windowInfo.image.name)
            .bindBlob(9, entry.windowInfo.image.bytes)
            .bindInt64(10, entry.windowInfo.image.encoded ? 1 : 0)
            .bindInt64(11, toMillis(entry.timestamp.capture))
            .bindInt64(12, entry.timestamp.duration.count());
        insertEntry_.run();
    }

public:
    explicit Impl(fs::path dbFile)
        : dbFile_(std::move(dbFile))
        , db_(dbFile_)
    {
        // WAL: readers and the writer don't block each other. NORMAL: cheaper
        // commits, durability still covered by WAL's checkpoint.
        db_.exec("PRAGMA journal_mode = WAL;"
                 "PRAGMA synchronous = NORMAL;"
                 "PRAGMA foreign_keys = ON;");
        db_.exec(SCHEMA_SQL);
        migrateSchema();

        // ON CONFLICT ... DO UPDATE ... RETURNING id is a get-or-create in one
        // round trip.
        upsertUser_ =
            db_.prepare("INSERT INTO users(username) VALUES (?1) "
                        "ON CONFLICT(username) DO UPDATE SET username = username "
                        "RETURNING id");

        upsertProcess_ =
            db_.prepare("INSERT INTO processes(path, sha256) VALUES (?1, ?2) "
                        "ON CONFLICT(path, sha256) DO UPDATE SET path = path "
                        "RETURNING id");

        insertEntry_ = db_.prepare(
            "INSERT INTO entries(user_id, process_id, window_title, rect_x, rect_y, "
            "rect_w, rect_h, image_name, image_bytes, image_encoded, captured_at, "
            "duration_ms) VALUES (?1,?2,?3,?4,?5,?6,?7,?8,?9,?10,?11,?12) "
            "ON CONFLICT(user_id, captured_at) DO NOTHING");

        selectUsers_ = db_.prepare("SELECT username FROM users ORDER BY username");

        selectEntries_ = db_.prepare(
            "SELECT e.captured_at, e.duration_ms, e.window_title, e.rect_x, e.rect_y, "
            "e.rect_w, e.rect_h, e.image_name, e.image_bytes, e.image_encoded, "
            "p.path, p.sha256 "
            "FROM entries e "
            "JOIN processes p ON p.id = e.process_id "
            "JOIN users u ON u.id = e.user_id "
            "WHERE u.username = ?1 AND e.captured_at BETWEEN ?2 AND ?3 "
            "ORDER BY e.captured_at ASC, e.id ASC");
    }

    std::int64_t userVersion() const
    {
        auto stmt = db_.prepare("PRAGMA user_version");
        return stmt.step() ? stmt.columnInt64(0) : 0;
    }

    // Brings an existing database up to SCHEMA_VERSION, one step at a time.
    // Each step runs in its own transaction together with its version bump.
    void migrateSchema()
    {
        if (userVersion() < 1)
        {
            // v1: an entry is identified by user and capture time, so re-adding
            // it (e.g. re-running a migration) is a no-op. Databases created
            // before may already hold duplicates; keep the oldest of each.
            sqlite::Transaction txn(db_);
            auto dedup = db_.prepare("DELETE FROM entries WHERE id NOT IN ("
                                     "SELECT MIN(id) FROM entries "
                                     "GROUP BY user_id, captured_at)");
            dedup.run();
            const auto removed = db_.changes();
            db_.exec("DROP INDEX IF EXISTS ux_entries_identity;"
                     "DROP INDEX IF EXISTS idx_entries_user_time;"
                     "CREATE UNIQUE INDEX IF NOT EXISTS ux_entries_user_time ON "
                     "entries(user_id, captured_at);"
                     "PRAGMA user_version = 1;");
            txn.commit();

            if (removed > 0)
            {
                spdlog::warn("{}: removed {} duplicate entries while upgrading "
                             "the schema to v1",
                             dbFile_,
                             removed);
            }
        }
    }

    const fs::path& dbFile() const noexcept
    {
        return dbFile_;
    }

    // One transaction for the whole span: get-or-create + insert must all land
    // or all roll back, and a single commit is far cheaper than one per entry.
    void addAll(std::span<const Entry> entries)
    {
        const std::scoped_lock lock(mtx_);

        try
        {
            sqlite::Transaction txn(db_);
            for (const auto& entry : entries)
            {
                insert(entry);
            }
            txn.commit();
        }
        catch (...)
        {
            // Ids resolved inside the rolled-back transaction may not exist.
            userIds_.clear();
            processIds_.clear();
            throw;
        }
    }

    void queryUsers(const UserCb& cb) const
    {
        const std::scoped_lock lock(mtx_);

        selectUsers_.reset();
        while (selectUsers_.step())
        {
            if (!cb(selectUsers_.columnText(0)))
            {
                return;
            }
        }
    }

    void queryEntries(const Filter& filter, const EntryCb& cb) const
    {
        if (filter.from() > filter.to())
        {
            return;
        }

        const std::scoped_lock lock(mtx_);

        selectEntries_.reset();
        selectEntries_.bindText(1, filter.username())
            .bindInt64(2, toMillis(filter.from()))
            .bindInt64(3, toMillis(filter.to()));

        Entry entry;
        while (selectEntries_.step())
        {
            hydrateEntry(selectEntries_, filter.username(), entry);
            if (!cb(entry))
            {
                return;
            }
        }
    }
};

SqliteRepository::SqliteRepository(fs::path dbFile)
    : pimpl_(std::make_unique<Impl>(std::move(dbFile)))
{
}

SqliteRepository::~SqliteRepository() = default;

const fs::path& SqliteRepository::dbFile() const noexcept
{
    return pimpl_->dbFile();
}

void SqliteRepository::add(const Entry& entry)
{
    pimpl_->addAll({&entry, 1});
}

void SqliteRepository::addAll(std::span<const Entry> entries)
{
    pimpl_->addAll(entries);
}

void SqliteRepository::queryUsers(const UserCb& cb) const
{
    pimpl_->queryUsers(cb);
}

void SqliteRepository::queryEntries(const Filter& filter, const EntryCb& cb) const
{
    pimpl_->queryEntries(filter, cb);
}

} // namespace km
