#pragma once

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace tools::dups {

class IDeletionStrategy
{
public:
    virtual ~IDeletionStrategy() = default;

    virtual void remove(const fs::path& file) const = 0;
};


class PermanentDelete : public IDeletionStrategy
{
public:
    void remove(const fs::path& file) const override;
};


class BackupAndDelete : public IDeletionStrategy
{
    fs::path backupDir_;
    fs::path journalFilePath_;
    mutable std::ofstream journalFile_;

    std::ofstream& journal() const;

public:
    BackupAndDelete(fs::path backupDir);

    void remove(const fs::path& file) const override;

    const fs::path& journalFile() const;
};


class DryRunDelete : public IDeletionStrategy
{
public:
    void remove(const fs::path& file) const override;
};


} // namespace tools::dups