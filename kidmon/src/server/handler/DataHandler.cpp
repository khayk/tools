#include <kidmon/server/handler/DataHandler.h>
#include <kidmon/common/Utils.h>
#include <kidmon/data/Types.h>
#include <core/utils/File.h>
#include <core/utils/Crypto.h>
#include <core/utils/Str.h>

#include <nlohmann/json.hpp>
#include <fmt/format.h>
#include <spdlog/spdlog.h>

struct ReportDirs
{
    fs::path snapshotsDir;
    //fs::path dailyDir;
    //fs::path monthlyDir;
    //fs::path weeklyDir;
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

    const fs::path& reportsDir() const noexcept {
        return reportsDir_;
    }

    const ReportDirs& getUserDirs(const std::string& username)
    {
        if (auto it = dirs_.find(username); it != dirs_.end())
        {
            return it->second;
        }

        std::time_t t = std::time(0); // get time now
        std::tm* now = std::localtime(&t);

        fs::path userReportsRoot = fs::path(reportsDir_)
                                       .append(str::s2ws(username))
                                       .append(fmt::format("{}", now->tm_year + 1900));

        // Reports directory structure will look like this
        //
        // ...\kidmon\reports\user\YYYY\snapshots\MM.DD"
        //                             \daily\d-001.txt
        //                             \monthly\m-01.txt
        //                             \weekly\w-01.txt
        //                             \raw\r-001.dat

        ReportDirs dirs;

        dirs.snapshotsDir = userReportsRoot / "snapshots";
        //dirs.dailyDir = userReportsRoot / "daily";
        //dirs.monthlyDir = userReportsRoot / "monthly";
        //dirs.weeklyDir = userReportsRoot / "weekly";
        dirs.rawDir = userReportsRoot / "raw";

        fs::create_directories(dirs.snapshotsDir);
        //fs::create_directories(dirs.dailyDir);
        // fs::create_directories(dirs.monthlyDir);
        // fs::create_directories(dirs.weeklyDir);
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

DataHandler::DataHandler(IDataStorage& storage)
    : storage_(storage)
{
}


bool DataHandler::handle(const nlohmann::json& payload,
                         nlohmann::json& answer,
                         std::string& error)
{
    try
    {
        const auto& msg = payload["message"];

        Entry entry;
        fromJson(msg["entry"], entry);
        entry.username = msg["username"];

        if (entry.username.empty() ||
            entry.username != str::ws2s(sys::activeUserName()))
        {
            error = "Username mismatch";
            return false;
        }

        const std::string& imageName = entry.windowInfo.image.name;
        const std::string& imageBytes = entry.windowInfo.image.bytes;
        const bool hasSnapshot = !imageName.empty();

        if (hasSnapshot)
        {
            crypto::decodeBase64(imageBytes, buffer_);
            entry.windowInfo.image.encoded = false;
            entry.windowInfo.image.bytes = buffer_;
        }

        storage_.add(entry);

        return true;
    }
    catch (const std::exception& ex)
    {
        error = fmt::format("Unable to handle incoming payload: {}", ex.what());
    }

    return false;
}

class FileSystemStorage::Impl
{
    Dirs dirs_;

public:
    Impl(fs::path reportsDir)
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
        const auto rawname = fmt::format("raw-{:03}-{:02}{:02}.dat", day, tm.tm_mon + 1, tm.tm_mday);
        const auto rawfile = userDirs.rawDir / rawname;

        nlohmann::ordered_json js;
        Entry tmp = entry;
        tmp.windowInfo.image.bytes.clear();
        toJson(tmp, js);
        file::append(rawfile, js.dump().append(1, '\n'));
    }
};

FileSystemStorage::FileSystemStorage(fs::path reportsDir)
    : pimpl_(std::make_unique<Impl>(std::move(reportsDir)))
{
}

FileSystemStorage::~FileSystemStorage()
{
}

void FileSystemStorage::add(const Entry& entry)
{
    pimpl_->add(entry);
}
