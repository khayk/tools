#pragma once

#include <kidmon/data/Types.h>

#include <sqlite3.h>

#include <cstdint>
#include <string_view>

namespace km::sqlite {

class Statement;

/**
 * @brief RAII wrapper around a single sqlite3 connection.
 *
 * Not a general-purpose ORM -- just enough that callers never touch the raw
 * sqlite3_* API. Throws std::runtime_error on failure. Not internally
 * synchronized; callers must serialize concurrent access themselves.
 */
class Database
{
public:
    explicit Database(const fs::path& file);
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
    Database(Database&& other) noexcept;
    Database& operator=(Database&& other) noexcept;

    /// Runs statements with no parameters and no result rows (DDL, PRAGMAs).
    void exec(std::string_view sql) const;

    /// Compiles `sql` into a reusable, bindable Statement.
    [[nodiscard]] Statement prepare(std::string_view sql) const;

    [[nodiscard]] std::int64_t lastInsertRowId() const noexcept;

private:
    sqlite3* db_ {nullptr};
};

} // namespace km::sqlite
