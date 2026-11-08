#include <duplicates/DeletionStrategy.h>
#include <core/utils/File.h>
#include <core/utils/Crypto.h>
#include <core/utils/FmtExt.h>

#include <format>
#include <spdlog/spdlog.h>

#include <filesystem>
#include <system_error>

namespace tools::dups {
namespace {

fs::path createRunDir(const fs::path& backupDir)
{
    fs::create_directories(backupDir);

    const auto now = std::chrono::system_clock::now();
    const auto timeT = std::chrono::system_clock::to_time_t(now);
#ifdef _WIN32
    tm tm;
    localtime_s(&tm, &timeT);
#else
    const auto tm = *std::localtime(&timeT);
#endif
    const auto stamp = std::format("{:04}{:02}{:02}_{:02}{:02}{:02}",
                                   tm.tm_year + 1900,
                                   tm.tm_mon + 1,
                                   tm.tm_mday,
                                   tm.tm_hour,
                                   tm.tm_min,
                                   tm.tm_sec);

    // Each run gets its own directory so backups from different runs can never
    // overwrite one another. Without this, deleting the same source path in two
    // separate runs would collide on backupDir/<md5(parent)>/<filename> and the
    // later run would silently destroy the earlier backup. The counter suffix
    // disambiguates the rare case of two runs starting within the same second.
    auto runDir = backupDir / std::format("deleted_{}", stamp);
    for (int n = 1;; ++n)
    {
        std::error_code ec;
        if (fs::create_directory(runDir, ec))
        {
            return runDir; // created a fresh run directory
        }

        if (ec)
        {
            throw std::system_error(
                ec,
                std::format("Unable to create backup run directory under: {}",
                            backupDir));
        }

        // The directory already exists (no error): try the next suffixed name.
        runDir = backupDir / std::format("deleted_{}_{}", stamp, n);
    }
}

} // namespace

void PermanentDelete::remove(const fs::path& file) const
{
    fs::remove(file);
    spdlog::info("Deleted: {}", file);
}


BackupAndDelete::BackupAndDelete(fs::path backupDir)
    : backupDir_(std::move(backupDir))
    , runDir_(createRunDir(backupDir_))
    , journal_(runDir_ / JOURNAL_FILE_NAME)
{
}

BackupAndDelete::~BackupAndDelete()
{
    // Drop the run directory if nothing was backed up, so quitting without any
    // deletions does not leave empty directories behind. Removal of a non-empty
    // directory (a journal or backups exist) fails harmlessly.
    std::error_code ec;
    fs::remove(runDir_, ec);
}

const fs::path& BackupAndDelete::runDir() const noexcept
{
    return runDir_;
}

const fs::path& BackupAndDelete::journalFile() const
{
    return journal_.path();
}

void BackupAndDelete::remove(const fs::path& file) const
{
    if (!fs::exists(file))
    {
        return;
    }

    const auto absFile = file.is_absolute() ? file : fs::absolute(file);
    const auto parentPath = absFile.parent_path();
    const auto hash = core::crypto::md5(core::file::path2s(parentPath));
    const auto backupFilePath = runDir_ / hash / absFile.filename();

    fs::create_directory(runDir_ / hash);

    // Record the move before performing it: if the app dies in between, restore
    // finds an entry whose backup is missing and skips it, whereas the reverse
    // order could leave a backup that no journal entry points to.
    journal_.append({absFile, backupFilePath});

    std::error_code ec;
    fs::rename(absFile, backupFilePath, ec);

    if (ec)
    {
        // Move fails, it can be for exapmle because of cross device operation,
        // no need to log error, try copying and deleting
        fs::copy_file(absFile, backupFilePath, fs::copy_options::overwrite_existing);
        fs::remove(absFile);
    }

    spdlog::info("Moved: {} to {}", absFile, backupFilePath);
}

void DryRunDelete::remove(const fs::path& file) const
{
    spdlog::info("Would delete: {}", file);
}

} // namespace tools::dups
