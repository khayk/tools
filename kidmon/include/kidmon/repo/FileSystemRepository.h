#pragma once

#include "Repository.h"

class FileSystemRepository : public IRepository
{
public:
    explicit FileSystemRepository(fs::path reportsDir);
    ~FileSystemRepository() override;

    void add(const Entry& entry) override;

    void queryUsers(UserCb cb) const override;

    void queryEntries(const Filter& filter, EntryCb cb) const override;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};