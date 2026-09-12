#include "SqliteStatement.h"

#include <format>
#include <stdexcept>
#include <utility>

namespace km::sqlite {

namespace {

[[noreturn]] void throwSqliteError(sqlite3_stmt* stmt, std::string_view what)
{
    sqlite3* db = sqlite3_db_handle(stmt);
    const std::string msg = db != nullptr ? sqlite3_errmsg(db) : "no connection";
    throw std::runtime_error(std::format("{}: {}", what, msg));
}

void checkBind(sqlite3_stmt* stmt, int rc, int index)
{
    if (rc != SQLITE_OK)
    {
        throwSqliteError(stmt, std::format("Failed to bind parameter {}", index));
    }
}

} // namespace

Statement::Statement(sqlite3_stmt* stmt) noexcept
    : stmt_(stmt)
{
}

Statement::~Statement()
{
    sqlite3_finalize(stmt_);
}

Statement::Statement(Statement&& other) noexcept
    : stmt_(std::exchange(other.stmt_, nullptr))
{
}

Statement& Statement::operator=(Statement&& other) noexcept
{
    if (this != &other)
    {
        finalizeAndTake(std::exchange(other.stmt_, nullptr));
    }
    return *this;
}

void Statement::finalizeAndTake(sqlite3_stmt* stmt) noexcept
{
    sqlite3_finalize(stmt_);
    stmt_ = stmt;
}

Statement& Statement::bindInt64(int index, std::int64_t value)
{
    checkBind(stmt_, sqlite3_bind_int64(stmt_, index, value), index);
    return *this;
}

Statement& Statement::bindText(int index, std::string_view value)
{
    // SQLITE_TRANSIENT: sqlite copies the bytes, so `value` need not outlive the call.
    checkBind(stmt_,
              sqlite3_bind_text(stmt_,
                                index,
                                value.data(),
                                static_cast<int>(value.size()),
                                SQLITE_TRANSIENT),
              index);
    return *this;
}

Statement& Statement::bindBlob(int index, std::string_view bytes)
{
    if (bytes.empty())
    {
        // sqlite3_bind_blob with null + nonzero length is undefined.
        checkBind(stmt_, sqlite3_bind_zeroblob(stmt_, index, 0), index);
        return *this;
    }

    checkBind(stmt_,
              sqlite3_bind_blob(stmt_,
                                index,
                                bytes.data(),
                                static_cast<int>(bytes.size()),
                                SQLITE_TRANSIENT),
              index);
    return *this;
}

Statement& Statement::bindNull(int index)
{
    checkBind(stmt_, sqlite3_bind_null(stmt_, index), index);
    return *this;
}

bool Statement::step()
{
    const int rc = sqlite3_step(stmt_);
    if (rc == SQLITE_ROW)
    {
        return true;
    }

    if (rc == SQLITE_DONE)
    {
        return false;
    }

    throwSqliteError(stmt_, "Failed to step statement");
}

void Statement::run()
{
    while (step())
    {
    }
}

void Statement::reset()
{
    if (sqlite3_reset(stmt_) != SQLITE_OK)
    {
        throwSqliteError(stmt_, "Failed to reset statement");
    }
    sqlite3_clear_bindings(stmt_);
}

std::int64_t Statement::columnInt64(int index) const
{
    return sqlite3_column_int64(stmt_, index);
}

std::string Statement::columnText(int index) const
{
    const auto* text =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt_, index));
    const auto bytes = sqlite3_column_bytes(stmt_, index);
    return text != nullptr ? std::string(text, static_cast<std::size_t>(bytes))
                           : std::string();
}

std::string Statement::columnBlob(int index) const
{
    const auto* blob =
        reinterpret_cast<const char*>(sqlite3_column_blob(stmt_, index));
    const auto bytes = sqlite3_column_bytes(stmt_, index);
    return blob != nullptr ? std::string(blob, static_cast<std::size_t>(bytes))
                           : std::string();
}

bool Statement::columnIsNull(int index) const
{
    return sqlite3_column_type(stmt_, index) == SQLITE_NULL;
}

} // namespace km::sqlite
