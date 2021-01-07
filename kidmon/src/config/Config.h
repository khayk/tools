#pragma once

#include "Constants.h"
#include <string>

struct Config
{
    std::string appDataDir;
    std::string reportsDir;
    std::string logsDir;

    uint32_t activityCheckIntervalMs{ 0 };
    uint32_t snapshotIntervalMs{ 0 };

    void applyDefaults();
    void applyOverrides(const std::wstring& /*filePath*/);
};
