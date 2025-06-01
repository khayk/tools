#include <duplicates/DeletionStrategy.h>
#include <core/utils/File.h>
#include <core/utils/Crypto.h>
#include <core/utils/FmtExt.h>

#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include <filesystem>

namespace tools::dups {

void PermanentDelete::apply(const fs::path& file)
{
    fs::remove(file);
    spdlog::info("Deleted: {}", file);
}


BackupAndDelete::BackupAndDelete(fs::path backupDir)
    : backupDir_(std::move(backupDir))
{
    if (!fs::exists(backupDir_))
    {
        fs::create_directories(backupDir_);
    }

    const auto now = std::chrono::system_clock::now();
    const auto timeT = std::chrono::system_clock::to_time_t(now);
#ifdef _WIN32
    tm tm;
    localtime_s(&tm, &timeT);
#else
    const auto tm = *std::localtime(&timeT);
#endif
    const auto date =
        fmt::format("{:04}{:02}{:02}", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
    const auto time = fmt::format("{:02}{:02}{:02}", tm.tm_hour, tm.tm_min, tm.tm_sec);
    const auto fileName = fmt::format("deleted_files_{}_{}.log", date, time);
    journalFilePath_ = backupDir_ / fileName;
}

std::ofstream& BackupAndDelete::journal()
{
    if (!journalFile_.is_open())
    {
        journalFile_.open(journalFilePath_, std::ios::ate);

        if (!journalFile_)
        {
            throw std::system_error(
                std::make_error_code(std::errc::no_such_file_or_directory),
                fmt::format("Unable to open file: {}", journalFilePath_));
        }
    }

    return journalFile_;
}

const fs::path& BackupAndDelete::journalFile() const
{
    return journalFilePath_;
}

void BackupAndDelete::apply(const fs::path& file)
{
    if (!fs::exists(file))
    {
        return;
    }

    const auto absFile = file.is_absolute() ? file : fs::absolute(file);
    const auto parentPath = absFile.parent_path();
    const auto hash = crypto::md5(file::path2s(parentPath));
    const auto backupFilePath = backupDir_ / hash / absFile.filename();

    fs::create_directory(backupDir_ / hash);
    journal() << absFile << "|" << backupFilePath << '\n';

    fs::rename(absFile, backupFilePath);
    spdlog::info("Moved: {} to {}", absFile, backupFilePath);
}

void DryRunDelete::apply(const fs::path& file)
{
    spdlog::info("Would delete: {}", file);
}

} // namespace tools::dups
