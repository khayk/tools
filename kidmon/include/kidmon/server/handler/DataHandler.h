#pragma once

#include "MsgHandler.h"
#include <filesystem>
#include <unordered_map>

namespace fs = std::filesystem;

struct ReportDirs
{
    fs::path snapshotsDir;
    fs::path dailyDir;
    fs::path monthlyDir;
    fs::path weeklyDir;
    fs::path rawDir;
};

class DataHandler : public MsgHandler
{
    fs::path reportsDir_;
    std::unordered_map<std::wstring, ReportDirs> dirs_;

    const ReportDirs& getActiveUserDirs();

public:
    DataHandler(fs::path reportsDir);

    bool handle(const nlohmann::json& payload,
                nlohmann::json& answer,
                std::string& error) override;
};
