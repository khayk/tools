#pragma once

#include "Repository.h"

#include <cstdint>

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
    enum class Mode : std::uint8_t
    {
        ReadWrite, // creates the file and upgrades its schema if needed
        ReadOnly,  // the file must exist and is never modified; add() throws
    };

    explicit SqliteRepository(fs::path dbFile, Mode mode = Mode::ReadWrite);
    ~SqliteRepository() override;

    [[nodiscard]] const fs::path& dbFile() const noexcept;

    void add(const Entry& entry) override;
    void addAll(std::span<const Entry> entries) override;
    bool atomicAddAll() const noexcept override; // one transaction per call
    void queryUsers(const UserCb& cb) const override;
    void queryEntries(const Filter& filter, const EntryCb& cb) const override;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace km
