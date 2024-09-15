#pragma once

#include "Repository.h"

class FileSystemRepository : public IRepository
{
public:
    explicit FileSystemRepository(fs::path reportsDir);
    ~FileSystemRepository() override;

    const fs::path& reportsDir() const noexcept;

    // Overrides
    void add(const Entry& entry) override;

    void queryUsers(const UserCb& cb) const override;

    void queryEntries(const Filter& filter, const EntryCb& cb) const override;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};