#pragma once

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace tools::dups
{

class DeletionStrategy
{
public:
    virtual ~DeletionStrategy() = default;

    virtual void apply(const fs::path& file) = 0;
};


class DeletePermanently : public DeletionStrategy
{
public:
    void apply(const fs::path& file) override;
};


class BackupAndDelete : public DeletionStrategy
{
    fs::path backupDir_;
    fs::path journalFilePath_;
    std::ofstream journalFile_;

public:
    BackupAndDelete(fs::path backupDir);

    void apply(const fs::path& file) override;

    const fs::path& journalFile() const;
};


} // namespace tools::dups