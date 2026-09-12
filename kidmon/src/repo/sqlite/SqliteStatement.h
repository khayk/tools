#pragma once

#include <sqlite3.h>

#include <cstdint>
#include <string>
#include <string_view>

namespace km::sqlite {

/**
 * @brief RAII wrapper around a prepared sqlite3_stmt.
 *
 * Bind indices are 1-based, matching sqlite's own convention. Move-only;
 * finalizes on destruction. Reset and reuse an instance across calls instead
 * of re-preparing the SQL text each time.
 */
class Statement
{
public:
    Statement() = default;
    explicit Statement(sqlite3_stmt* stmt) noexcept;
    ~Statement();

    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;
    Statement(Statement&& other) noexcept;
    Statement& operator=(Statement&& other) noexcept;

    Statement& bindInt64(int index, std::int64_t value);
    Statement& bindText(int index, std::string_view value);
    Statement& bindBlob(int index, std::string_view bytes);
    Statement& bindNull(int index);

    /// Advances execution; true while a row is available.
    bool step();

    /// Runs a statement that produces no rows (INSERT/UPDATE/DELETE/DDL).
    void run();

    /// Clears bound parameters and rewinds, ready for reuse.
    void reset();

    [[nodiscard]] std::int64_t columnInt64(int index) const;
    [[nodiscard]] std::string columnText(int index) const;
    [[nodiscard]] std::string columnBlob(int index) const;
    [[nodiscard]] bool columnIsNull(int index) const;

private:
    // Finalizes whatever this instance currently owns (if anything) and takes
    // ownership of `stmt` instead. Used by the move operations.
    void finalizeAndTake(sqlite3_stmt* stmt) noexcept;

    sqlite3_stmt* stmt_ {nullptr};
};

} // namespace km::sqlite
