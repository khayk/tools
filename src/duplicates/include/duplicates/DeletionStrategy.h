#pragma once

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace tools::dups {

class IDeletionStrategy
{
public:
    virtual ~IDeletionStrategy() = default;

    virtual void apply(const fs::path& file) = 0;
};


class PermanentDelete : public IDeletionStrategy
{
public:
    void apply(const fs::path& file) override;
};


class BackupAndDelete : public IDeletionStrategy
{
    fs::path backupDir_;
    fs::path journalFilePath_;
    std::ofstream journalFile_;

    std::ofstream& journal();

public:
    BackupAndDelete(fs::path backupDir);

    void apply(const fs::path& file) override;

    const fs::path& journalFile() const;
};


class DryRunDelete : public IDeletionStrategy
{
public:
    void apply(const fs::path& file) override;
};


} // namespace tools::dups