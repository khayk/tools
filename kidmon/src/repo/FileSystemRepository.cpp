#include <kidmon/repo/FileSystemRepository.h>
#include <kidmon/common/Utils.h>
#include <kidmon/data/Constants.h>

#include <core/utils/File.h>
#include <core/utils/Str.h>
#include <core/utils/FmtExt.h>
#include <core/utils/Number.h>
#include <core/utils/StopWatch.h>

#include "RawEntryDto.h"
#include "RawFileRange.h"

#include <algorithm>
#include <format>
#include <vector>
#include <nlohmann/json.hpp>
#include <glaze/glaze.hpp>
#include <spdlog/spdlog.h>

using namespace core;

namespace km {

namespace {

struct ReportDirs
{
    fs::path snapshotsDir;
    fs::path rawDir;
    // fs::path dailyDir;
    // fs::path monthlyDir;
    // fs::path weeklyDir;
};

int yearFromTimeT(const time_t tt, bool throwIfInvalid)
{
    tm tm {};

    if (!utl::timet2tm(tt, tm))
    {
        if (throwIfInvalid)
        {
            throw std::runtime_error(
                std::format("Unable to convert '{}' to local time", tt));
        }

        return 0;
    }

    return tm.tm_year + 1900;
}

int yearFromTimePoint(const TimePoint tp, bool throwIfInvalid)
{
    return yearFromTimeT(SystemClock::to_time_t(tp), throwIfInvalid);
}

class Dirs
{
    mutable std::unordered_map<std::string, ReportDirs> dirs_;
    fs::path reportsDir_;

public:
    explicit Dirs(fs::path reportsDir)
        : reportsDir_(std::move(reportsDir))
    {
    }

    const fs::path& reportsDir() const noexcept
    {
        return reportsDir_;
    }

    fs::path getUserDir(const std::string& username) const
    {
        if (username.empty())
        {
            throw std::runtime_error("Empty username");
        }

        return fs::path(reportsDir_).append(str::s2ws(username)).lexically_normal();
    }

    const ReportDirs& dataDirs(const std::string& username, int year) const
    {
        const auto key = std::format("{}_{}", username, year);
        if (auto it = dirs_.find(key); it != dirs_.end())
        {
            return it->second;
        }

        fs::path userReportsRoot =
            getUserDir(username).append(std::format("{}", year));

        // Reports directory structure looks like this
        //
        // ...\kidmon\reports\user\YYYY\snapshots\MM.DD"
        //                             \daily\d-001.txt
        //                             \monthly\m-01.txt
        //                             \weekly\w-01.txt
        //                             \raw\r-001.dat

        ReportDirs dirs;
        dirs.snapshotsDir = userReportsRoot / "snapshots";
        // dirs.dailyDir = userReportsRoot / "daily";
        // dirs.monthlyDir = userReportsRoot / "monthly";
        // dirs.weeklyDir = userReportsRoot / "weekly";
        dirs.rawDir = userReportsRoot / "raw";

        fs::create_directories(dirs.snapshotsDir);
        // fs::create_directories(dirs.dailyDir);
        //  fs::create_directories(dirs.monthlyDir);
        //  fs::create_directories(dirs.weeklyDir);
        fs::create_directories(dirs.rawDir);

        auto [it, _] = dirs_.emplace(key, std::move(dirs));

        return it->second;
    }
};


// Snapshot file names arrive from the (potentially untrusted) agent and are
// joined onto snapshotsDir to form the write path. Reject anything that is not
// a plain, single-component file name so a peer cannot escape the directory via
// path separators, parent references or absolute paths (path traversal).
void validateSnapshotName(const std::string& name)
{
    const fs::path p(name);

    const bool illegal = name.empty() || name == "." || name == ".." ||
                         p.filename() != p || p.has_root_path() ||
                         name.contains('/') || name.contains('\\');

    if (illegal)
    {
        throw std::runtime_error(
            std::format("Illegal snapshot file name: '{}'", name));
    }
}

std::string buildRawFilename(const TimePoint tp)
{
    const auto tt = SystemClock::to_time_t(tp);
    std::tm tm {};

    if (!utl::timet2tm(tt, tm))
    {
        return "";
    }

    const auto day = utl::daysSinceYearStart(tt);
    return std::format("raw-{:03}-{:02}{:02}.dat", day, tm.tm_mon + 1, tm.tm_mday);
}

void readEntries(const std::string& username, const fs::path& file, const EntryCb& cb)
{
    detail::EntryDto dto;
    Entry entry;
    entry.username = username;

    file::readLines(file, [&cb, &dto, &entry, &file](const std::string& line) {
        const auto sv = str::trim(line);
        if (sv.empty())
        {
            return true;
        }

        // sv is a view into the (possibly trailing-trimmed) line, so it is not
        // guaranteed null-terminated. Tolerate extra/unknown keys for forward
        // compatibility, but reject a line missing required fields so a partial
        // record is skipped rather than surfacing as a default-valued entry.
        constexpr glz::opts opts {.null_terminated = false,
                                  .error_on_unknown_keys = false,
                                  .error_on_missing_keys = true};
        if (const auto ec = glz::read<opts>(dto, sv))
        {
            // A malformed line must not surface as a default-constructed entry
            // in the results; skip it (keep reading subsequent lines) instead.
            spdlog::warn("Skipping malformed entry in {}: {}",
                         file,
                         glz::format_error(ec, sv));
            return true;
        }

        detail::toEntry(dto, entry);
        return cb(entry);
    });
}

bool queryRawDataDir(const Filter& filter,
                     const EntryCb& cb,
                     const Dirs& dirs_,
                     const int year)
{
    const int yearFrom = yearFromTimePoint(filter.from(), false);
    const int yearTo = yearFromTimePoint(filter.to(), false);

    spdlog::info("Reading data for year: {}", year);

    if (!detail::yearInRange(year, yearFrom, yearTo))
    {
        return true;
    }

    const auto fnFrom = buildRawFilename(filter.from());
    const auto fnTo = buildRawFilename(filter.to());
    const auto& dataDirs = dirs_.dataDirs(filter.username(), year);
    const fs::path& rawDir = dataDirs.rawDir;

    spdlog::info("Directory selected for scanning: {}", rawDir);

    // directory_iterator yields entries in an unspecified order, but a callback
    // may stop the query early and expects to see entries oldest-first. Collect
    // the in-range day files and sort them so the visit order is deterministic
    // and chronological (file names sort chronologically within a year).
    std::vector<fs::path> files;
    for (const auto& it : fs::directory_iterator(rawDir))
    {
        if (!it.is_regular_file())
        {
            continue;
        }

        const auto fn = it.path().filename().string();
        if (detail::rawFileInRange(fn, year, yearFrom, fnFrom, yearTo, fnTo))
        {
            files.push_back(it.path());
        }
    }
    std::ranges::sort(files);

    bool keepGoing = true;
    for (const auto& file : files)
    {
        if (!keepGoing)
        {
            break;
        }

        StopWatch timer;
        readEntries(filter.username(),
                    file,
                    [&keepGoing, &cb, &filter](Entry& entry) {
                        if (entry.timestamp.capture >= filter.from() &&
                            entry.timestamp.capture <= filter.to())
                        {
                            keepGoing = cb(entry);
                        }

                        return keepGoing;
                    });
        spdlog::debug("File: '{}' is processed in {}",
                      file.filename(),
                      str::humanizeDuration(timer.elapsed()));
    }

    return keepGoing;
}

} // namespace

class FileSystemRepository::Impl
{
    Dirs dirs_;

public:
    explicit Impl(fs::path reportsDir)
        : dirs_ {std::move(reportsDir)}
    {
    }

    const fs::path& reportsDir() const noexcept
    {
        return dirs_.reportsDir();
    }

    void add(const Entry& entry)
    {
        const int year = yearFromTimePoint(entry.timestamp.capture, true);
        const auto& userDirs = dirs_.dataDirs(entry.username, year);
        const auto& bytes = entry.windowInfo.image.bytes;

        if (!bytes.empty())
        {
            validateSnapshotName(entry.windowInfo.image.name);
            const auto imagePath = userDirs.snapshotsDir / entry.windowInfo.image.name;
            file::write(imagePath, bytes.data(), bytes.size());
        }

        const auto rawName = buildRawFilename(entry.timestamp.capture);
        const auto rawFile = userDirs.rawDir / rawName;

        nlohmann::ordered_json js;
        toJson(entry, js, false);
        file::append(rawFile, js.dump().append(1, '\n'));
    }

    void queryUsers(const UserCb& cb) const
    {
        for (const auto& it : fs::directory_iterator(dirs_.reportsDir()))
        {
            if (it.is_directory())
            {
                if (!cb(file::path2s(it.path().filename())))
                {
                    // Requested to stop an enumeration
                    return;
                }
            }
        }
    }

    void queryEntries(const Filter& filter, const EntryCb& cb) const
    {
        if (filter.from() > filter.to())
        {
            return;
        }

        const auto userDir = dirs_.getUserDir(filter.username()).lexically_normal();

        std::vector<int> years;
        for (const auto& it : fs::directory_iterator(userDir))
        {
            if (!it.is_directory())
            {
                continue;
            }

            // Only YYYY directories are valid here; a stray non-numeric folder
            // must be skipped, not abort the whole query (std::stoi would throw).
            const auto year = core::num::s2num<int>(it.path().filename().string(), 0);
            if (year > 0)
            {
                years.push_back(year);
            }
            else
            {
                spdlog::warn("Skipping non-year directory: {}", it.path());
            }
        }
        std::ranges::sort(years);

        for (const int year : years)
        {
            if (!queryRawDataDir(filter, cb, dirs_, year))
            {
                return;
            }
        }
    }
};

FileSystemRepository::FileSystemRepository(fs::path reportsDir)
    : pimpl_(std::make_unique<Impl>(std::move(reportsDir)))
{
}

FileSystemRepository::~FileSystemRepository() = default;

const fs::path& FileSystemRepository::reportsDir() const noexcept
{
    return pimpl_->reportsDir();
}

void FileSystemRepository::add(const Entry& entry)
{
    pimpl_->add(entry);
}

void FileSystemRepository::queryUsers(const UserCb& cb) const
{
    pimpl_->queryUsers(cb);
}

void FileSystemRepository::queryEntries(const Filter& filter, const EntryCb& cb) const
{
    pimpl_->queryEntries(filter, cb);
}

} // namespace km
