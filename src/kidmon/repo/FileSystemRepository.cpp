#include <kidmon/repo/FileSystemRepository.h>
#include <kidmon/common/Utils.h>

#include <core/utils/File.h>
#include <core/utils/Str.h>

#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <glaze/glaze.hpp>

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
                fmt::format("Unable to convert '{}' to local time", tt));
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
    const fs::path reportsDir_;

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
        const auto key = fmt::format("{}_{}", username, year);
        if (auto it = dirs_.find(key); it != dirs_.end())
        {
            return it->second;
        }

        fs::path userReportsRoot =
            getUserDir(username).append(fmt::format("{}", year));

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

        auto [it, ok] = dirs_.emplace(key, std::move(dirs));

        if (!ok)
        {
            static ReportDirs s_dirs;

            return s_dirs;
        }

        return it->second;
    }
};


std::string buildRawFilename(const TimePoint tp)
{
    const auto tt = SystemClock::to_time_t(tp);
    std::tm tm {};

    if (!utl::timet2tm(tt, tm))
    {
        return "";
    }

    const auto day = utl::daysSinceYearStart(tt);
    return fmt::format("raw-{:03}-{:02}{:02}.dat", day, tm.tm_mon + 1, tm.tm_mday);
}

template <typename T>
T getAs(const glz::json_t& js)
{
    const auto value = js.get<double>();
    return static_cast<T>(value);
}

void readEntries(const std::string& username,
                 const fs::path& file,
                 const EntryCb& cb)
{
    Entry entry;
    glz::json_t json {};

    entry.username = username;

    file::readLines(file, [&cb, &entry, &json](const std::string& line) {
        const auto sv = str::trim(line);
        if (glz::read_json(json, sv))
        {
            return true;
        }

        try
        {
            auto& proc = json["proc"];
            entry.processInfo.processPath = proc["path"].get<std::string>();
            entry.processInfo.sha256 = proc["sha256"].get<std::string>();

            auto& wnd = json["wnd"];
            entry.windowInfo.title = wnd["title"].get<std::string>();

            const Point leftTop(getAs<int>(wnd["lt"][0]),
                                getAs<int>(wnd["lt"][1]));
            const Dimensions dimensions(getAs<uint32_t>(wnd["wh"][0]),
                                        getAs<uint32_t>(wnd["wh"][1]));
            entry.windowInfo.placement = Rect(leftTop, dimensions);

            auto& img = wnd["img"];
            entry.windowInfo.image.name = img["name"].get<std::string>();
            entry.windowInfo.image.bytes = img["bytes"].get<std::string>();
            entry.windowInfo.image.encoded = img["encoded"].get<bool>();

            const auto& ts = json["ts"];
            entry.timestamp.capture =
                TimePoint(std::chrono::milliseconds(getAs<long long>(ts["when"])));
            entry.timestamp.duration =
                std::chrono::milliseconds(getAs<long long>(ts["dur"]));
        }
        catch (const std::exception&)
        {
            entry = Entry();
        }

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

    if ((yearTo != 0 && yearTo < year) || (yearFrom != 0 && year < yearFrom))
    {
        return true;
    }

    bool keepGoing = true;
    const auto fnFrom = buildRawFilename(filter.from());
    const auto fnTo = buildRawFilename(filter.to());
    const auto& dataDirs = dirs_.dataDirs(filter.username(), year);
    const fs::path& rawDir = dataDirs.rawDir;

    for (const auto& it : fs::directory_iterator(rawDir))
    {
        const auto fn = it.path().filename();

        if ((yearFrom == year && fn < fnFrom) || (yearTo == year && fn > fnTo))
        {
            continue;
        }

        if ((yearFrom == yearTo) &&
            ((!fnFrom.empty() && fn < fnFrom) || (!fnTo.empty() && fn > fnTo) ||
                !keepGoing || !it.is_regular_file()))
        {
            continue;
        }

        readEntries(filter.username(),
                    it.path(),
                    [&keepGoing, &cb, &filter](Entry& entry) {
                        if (entry.timestamp.capture >= filter.from() &&
                            entry.timestamp.capture <= filter.to())
                        {
                            keepGoing = cb(entry);
                        }

                        return keepGoing;
                    });
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
            const auto imagePath = userDirs.snapshotsDir / entry.windowInfo.image.name;
            file::write(imagePath, bytes.data(), bytes.size());
        }

        const auto rawName = buildRawFilename(entry.timestamp.capture);
        const auto rawFile = userDirs.rawDir / rawName;

        nlohmann::ordered_json js;
        Entry tmp = entry;
        tmp.windowInfo.image.bytes.clear();
        toJson(tmp, js);
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

        for (const auto& it : fs::directory_iterator(userDir))
        {
            if (!it.is_directory())
            {
                continue;
            }

            const int year = std::stoi(it.path().filename().string());

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
