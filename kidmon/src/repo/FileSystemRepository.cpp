#include <kidmon/repo/FileSystemRepository.h>
#include <kidmon/common/Utils.h>

#include <core/utils/File.h>
#include <core/utils/Str.h>
#include <fmt/format.h>
#include <boost/iostreams/device/mapped_file.hpp>

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


bool isYearValid(int year) {
    return year >= 1970 && year <= 2100;
}

int yearFromTimeT(const time_t tt)
{
    const auto tm = utl::timet2tm(tt);

    return tm.tm_year + 1900;
}

int yearFromTimePoint(const TimePoint tp)
{
    return yearFromTimeT(SystemClock::to_time_t(tp));
}

int yearNow()
{
    return yearFromTimeT(std::time(0));
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
        return fs::path(reportsDir_).append(str::s2ws(username));
    }

    const ReportDirs& dataDirs(const std::string& username, int year) const
    {
        const auto key = fmt::format("{}_{}", username, year);
        if (auto it = dirs_.find(key); it != dirs_.end())
        {
            return it->second;
        }

        fs::path userReportsRoot = getUserDir(username).append(fmt::format("{}", year));

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


template <typename LineCb>
void readLines(const fs::path& file, const LineCb& cb)
{
    std::string line;
    boost::iostreams::mapped_file mmap(file.string(),
                                       boost::iostreams::mapped_file::readonly);
    auto f = mmap.const_data();
    auto e = f + mmap.size();

    while (f && f != e)
    {
        auto p = static_cast<const char*>(memchr(f, '\n', e - f));
        if (p)
        {
            line.assign(f, p);
            f = p + 1;
        }
        else
        {
            line.assign(f, e);
            f = p;
        }

        str::trim(line);
        if (line.empty())
        {
            continue;
        }

        if (!cb(line))
        {
            return;
        }
    }
}

} // namespace

class FileSystemRepository::Impl
{
    Dirs dirs_;

public:
    explicit Impl(fs::path reportsDir)
        : dirs_(std::move(reportsDir))
    {
    }

    const fs::path& reportsDir() const noexcept
    {
        return dirs_.reportsDir();
    }

    void add(const Entry& entry)
    {
        const int year = yearFromTimePoint(entry.timestamp.capture);
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
        for (auto const& it : fs::directory_iterator(dirs_.reportsDir()))
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
        
        for (auto const& it : fs::directory_iterator(dirs_.getUserDir(filter.username())))
        {
            if (!it.is_directory())
            {
                continue;
            }

            const int year = std::stoi(it.path().filename().string());
            const auto& dataDirs = dirs_.dataDirs(filter.username(), year);           
            
            if (!queryRawDataDir(filter, cb, dataDirs.rawDir))
            {
                return;
            }
        }
    }

private:
    template <typename T>
    T getAs(const glz::json_t& js) const
    {
        const auto value = js.get<double>();
        return static_cast<T>(value);
    }

    bool queryRawDataDir(const Filter& filter,
                         const EntryCb& cb,
                         const fs::path& rawDir) const
    {
        bool keepGoing    = true;
        const auto fnFrom = buildRawFilename(filter.from());
        const auto fnTo   = buildRawFilename(filter.to());

        for (auto const& it : fs::directory_iterator(rawDir))
        {
            const auto fn = it.path().filename();  
            if ((!fnFrom.empty() && fn < fnFrom) || 
                (!fnTo.empty() && fn > fnTo))
            {
                continue;
            }

            if (keepGoing && it.is_regular_file())
            {
                readEntries(it.path(), [&keepGoing, &cb, &filter](const Entry& entry) {
                    if (entry.timestamp.capture >= filter.from() && 
                        entry.timestamp.capture <= filter.to())
                    {
                        keepGoing = cb(entry);
                    }

                    return keepGoing;
                });
            }
        }

        return keepGoing;
    }

    void readEntries(const fs::path& file, const EntryCb& cb) const
    {
        Entry entry;
        glz::json_t json {};

        readLines(file, [&cb, &entry, &json, this](const std::string& line) {
            if (glz::read_json(json, line))
            {
                return true;
            }

            auto& proc = json["proc"];
            entry.processInfo.processPath = proc["path"].get<std::string>();

            auto& wnd = json["wnd"];
            entry.windowInfo.title = wnd["title"].get<std::string>();
            
            const Point leftTop(getAs<int>(wnd["lt"][0]), 
                                getAs<int>(wnd["lt"][1]));
            const Dimensions dimensions(getAs<int>(wnd["wh"][0]),
                                        getAs<int>(wnd["wh"][1]));
            entry.windowInfo.placement = Rect(leftTop, dimensions);

            const auto& ts = json["ts"];
            entry.timestamp.capture = TimePoint(std::chrono::milliseconds(getAs<long long>(ts["when"])));
            entry.timestamp.duration = std::chrono::milliseconds(getAs<long long>(ts["dur"]));

            return cb(entry);
        });
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
