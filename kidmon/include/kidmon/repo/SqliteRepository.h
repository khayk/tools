#pragma once

#include "Repository.h"

namespace km {

/**
 * @brief IRepository backed by a single-file SQLite database.
 *
 * One row per captured sample, nothing coalesced. Username and process
 * (path, sha256) are pulled into `users`/`processes` lookup tables and
 * referenced by id, keeping the high-volume `entries` table narrow.
 */
class SqliteRepository : public IRepository
{
public:
    explicit SqliteRepository(fs::path dbFile);
    ~SqliteRepository() override;

    [[nodiscard]] const fs::path& dbFile() const noexcept;

    void add(const Entry& entry) override;
    void queryUsers(const UserCb& cb) const override;
    void queryEntries(const Filter& filter, const EntryCb& cb) const override;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace km
