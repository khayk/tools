#include <kidmon/repo/FileSystemRepository.h>
#include <kidmon/common/Utils.h>

#include <core/utils/File.h>
#include <core/utils/Str.h>
#include <fmt/format.h>

#include <nlohmann/json.hpp>

namespace {

struct ReportDirs
{
    fs::path snapshotsDir;
    // fs::path dailyDir;
    // fs::path monthlyDir;
    // fs::path weeklyDir;
    fs::path rawDir;
};

class Dirs
{
    std::unordered_map<std::string, ReportDirs> dirs_;
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

    const ReportDirs& getUserDirs(const std::string& username)
    {
        if (auto it = dirs_.find(username); it != dirs_.end())
        {
            return it->second;
        }

        std::time_t t = std::time(0); // get time now
        std::tm now = utl::timet2tm(t);

        fs::path userReportsRoot = fs::path(reportsDir_)
                                       .append(str::s2ws(username))
                                       .append(fmt::format("{}", now.tm_year + 1900));

        // Reports directory structure will look like this
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

        auto [it, ok] = dirs_.emplace(username, std::move(dirs));

        if (!ok)
        {
            static ReportDirs s_dirs;

            return s_dirs;
        }

        return it->second;
    }
};
} // namespace

class FileSystemRepository::Impl
{
    Dirs dirs_;

public:
    explicit Impl(fs::path reportsDir)
        : dirs_(std::move(reportsDir))
    {
    }

    void add(const Entry& entry)
    {
        const auto& userDirs = dirs_.getUserDirs(entry.username);
        const auto& bytes = entry.windowInfo.image.bytes;

        if (!bytes.empty())
        {
            const auto file = userDirs.snapshotsDir / entry.windowInfo.image.name;
            file::write(file, bytes.data(), bytes.size());
        }

        auto tp = std::chrono::system_clock::to_time_t(entry.timestamp.capture);
        auto day = utl::daysSinceYearStart(tp);

        const auto tm = utl::timet2tm(tp);
        const auto rawName =
            fmt::format("raw-{:03}-{:02}{:02}.dat", day, tm.tm_mon + 1, tm.tm_mday);
        const auto rawFile = userDirs.rawDir / rawName;

        nlohmann::ordered_json js;
        Entry tmp = entry;
        tmp.windowInfo.image.bytes.clear();
        toJson(tmp, js);
        file::append(rawFile, js.dump().append(1, '\n'));
    }
};

FileSystemRepository::FileSystemRepository(fs::path reportsDir)
    : pimpl_(std::make_unique<Impl>(std::move(reportsDir)))
{
}

FileSystemRepository::~FileSystemRepository() = default;

void FileSystemRepository::add(const Entry& entry)
{
    pimpl_->add(entry);
}

void FileSystemRepository::queryUsers(UserCb cb) const
{
    std::ignore = cb;

    throw std::logic_error(fmt::format("Not implemented: {}", __func__));
}

void FileSystemRepository::queryEntries(const Filter& filter, EntryCb cb) const
{
    std::ignore = filter;
    std::ignore = cb;

    throw std::logic_error(fmt::format("Not implemented: {}", __func__));
}
