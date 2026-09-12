#pragma once

#include "SqliteDatabase.h"

namespace km::sqlite {

/**
 * @brief RAII "BEGIN IMMEDIATE" transaction guard.
 *
 * Rolls back on destruction unless commit() was called, so a multi-statement
 * write can never be left half-applied.
 */
class Transaction
{
public:
    explicit Transaction(const Database& db)
        : db_(db)
    {
        db_.exec("BEGIN IMMEDIATE");
    }

    ~Transaction()
    {
        if (!done_)
        {
            try // must not throw from a destructor
            {
                db_.exec("ROLLBACK");
            }
            catch (...)
            {
            }
        }
    }

    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;
    Transaction(Transaction&&) = delete;
    Transaction& operator=(Transaction&&) = delete;

    void commit()
    {
        db_.exec("COMMIT");
        done_ = true;
    }

private:
    const Database& db_;
    bool done_ {false};
};

} // namespace km::sqlite
