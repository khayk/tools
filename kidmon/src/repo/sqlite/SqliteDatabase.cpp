#include "SqliteDatabase.h"
#include "SqliteStatement.h"

#include <core/utils/File.h>

#include <format>
#include <stdexcept>
#include <utility>

namespace km::sqlite {

namespace {

[[noreturn]] void throwSqliteError(sqlite3* db, std::string_view what)
{
    const std::string msg = db != nullptr ? sqlite3_errmsg(db) : "no connection";
    throw std::runtime_error(std::format("{}: {}", what, msg));
}

} // namespace

Database::Database(const fs::path& file)
{
    // FULLMUTEX allows handing this connection to another thread later; it
    // does not permit concurrent use from two threads at once.
    const int flags =
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX;
    if (sqlite3_open_v2(core::file::path2s(file).c_str(), &db_, flags, nullptr) !=
        SQLITE_OK)
    {
        const std::string msg = std::format("Unable to open sqlite database '{}': {}",
                                            core::file::path2s(file),
                                            db_ != nullptr ? sqlite3_errmsg(db_) : "");
        if (db_ != nullptr)
        {
            sqlite3_close_v2(db_);
            db_ = nullptr;
        }
        throw std::runtime_error(msg);
    }
}

Database::~Database()
{
    if (db_ != nullptr)
    {
        sqlite3_close_v2(db_);
    }
}

Database::Database(Database&& other) noexcept
    : db_(std::exchange(other.db_, nullptr))
{
}

Database& Database::operator=(Database&& other) noexcept
{
    if (this != &other)
    {
        if (db_ != nullptr)
        {
            sqlite3_close_v2(db_);
        }
        db_ = std::exchange(other.db_, nullptr);
    }
    return *this;
}

void Database::exec(std::string_view sql) const
{
    char* errMsg = nullptr;
    // sqlite3_exec requires a null-terminated string; std::string_view is not
    // guaranteed to be, so materialize one.
    const std::string owned(sql);
    if (sqlite3_exec(db_, owned.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK)
    {
        const std::string msg = std::format("Failed to execute sql '{}': {}",
                                            owned,
                                            errMsg != nullptr ? errMsg : "");
        sqlite3_free(errMsg);
        throw std::runtime_error(msg);
    }
}

Statement Database::prepare(std::string_view sql) const
{
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_,
                           sql.data(),
                           static_cast<int>(sql.size()),
                           &stmt,
                           nullptr) != SQLITE_OK)
    {
        throwSqliteError(db_, std::format("Failed to prepare sql '{}'", sql));
    }
    return Statement(stmt);
}

std::int64_t Database::lastInsertRowId() const noexcept
{
    return sqlite3_last_insert_rowid(db_);
}

} // namespace km::sqlite
