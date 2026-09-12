#pragma once

#include <sqlite3.h>

#include <cstdint>
#include <memory>
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
    // unique_ptr makes ownership explicit and copy non-constructible/movable
    // "for free" -- no hand-written destructor or move operations needed.
    struct Finalizer
    {
        void operator()(sqlite3_stmt* stmt) const noexcept
        {
            sqlite3_finalize(stmt);
        }
    };

    std::unique_ptr<sqlite3_stmt, Finalizer> stmt_;
};

} // namespace km::sqlite
