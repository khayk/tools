#pragma once

#include <kidmon/geometry/Rect.h>
#include <filesystem>
#include <chrono>

#include <nlohmann/json_fwd.hpp>

using SystemClock = std::chrono::system_clock;
using TimePoint = SystemClock::time_point;
namespace fs = std::filesystem;

struct ProcessInfo
{
    fs::path processPath;
    std::string sha256;
};

struct WindowInfo
{
    std::string title;
    Rect placement;
};

struct Entry
{
    ProcessInfo processInfo;
    WindowInfo windowInfo;
    TimePoint timestamp;
};

void toJson(const ProcessInfo& pi, nlohmann::ordered_json& js);
void toJson(const WindowInfo& wi, nlohmann::ordered_json& js);
void toJson(const TimePoint& tp, nlohmann::ordered_json& js);
void toJson(const Entry& entry, nlohmann::ordered_json& js);