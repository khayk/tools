#include <kidmon/server/handler/DataHandler.h>
#include <kidmon/common/Utils.h>
#include <core/utils/File.h>

#include <nlohmann/json.hpp>
#include <fmt/format.h>

DataHandler::DataHandler(fs::path reportsDir)
    : reportsDir_(std::move(reportsDir))
{
}

const ReportDirs& DataHandler::getActiveUserDirs()
{
    std::wstring activeUserName = sys::activeUserName();

    if (auto it = dirs_.find(activeUserName); it != dirs_.end())
    {
        return it->second;
    }

    std::time_t t = std::time(0); // get time now
    std::tm* now = std::localtime(&t);

    fs::path userReportsRoot = fs::path(reportsDir_)
                                   .append(activeUserName)
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
    dirs.dailyDir = userReportsRoot / "daily";
    dirs.monthlyDir = userReportsRoot / "monthly";
    dirs.weeklyDir = userReportsRoot / "weekly";
    dirs.rawDir = userReportsRoot / "raw";

    fs::create_directories(dirs.snapshotsDir);
    fs::create_directories(dirs.dailyDir);
    fs::create_directories(dirs.monthlyDir);
    fs::create_directories(dirs.weeklyDir);
    fs::create_directories(dirs.rawDir);

    auto [it, ok] = dirs_.emplace(activeUserName, std::move(dirs));

    if (!ok)
    {
        static ReportDirs s_dirs;

        return s_dirs;
    }

    return it->second;
}

bool DataHandler::handle(const nlohmann::json& payload,
                         nlohmann::json& answer,
                         std::string& error)
{
    std::ignore = payload;
    std::ignore = answer;
    std::ignore = error;

    bool hasSnapshot = false;
    std::string imageName;
    std::string imageBytes;

    if (hasSnapshot)
    {
        const auto& userDirs = getActiveUserDirs();
        auto file = userDirs.snapshotsDir / imageName;
        
        file::write(file, imageBytes.data(), imageBytes.size());
    }

    answer = R"({})";



    return false;
}
