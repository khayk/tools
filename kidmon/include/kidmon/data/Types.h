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

    bool operator==(const ProcessInfo&) const = default;
};

struct Image
{
    std::string name;
    std::string bytes;
    bool encoded {false}; // base64 encoded

    bool operator==(const Image&) const = default;
};

struct WindowInfo
{
    Rect placement;
    Image image;
    std::string title;

    bool operator==(const WindowInfo&) const = default;
};

struct Timestamp
{
    TimePoint capture;
    std::chrono::milliseconds duration {};

    bool operator==(const Timestamp&) const = default;
};

struct Entry
{
    std::string username;
    ProcessInfo processInfo;
    WindowInfo windowInfo;
    Timestamp timestamp;

    bool operator==(const Entry&) const = default;
};

void toJson(const ProcessInfo& pi, nlohmann::ordered_json& js);
void toJson(const WindowInfo& wi, nlohmann::ordered_json& js);
void toJson(const Image& image, nlohmann::ordered_json& js);
void toJson(const Timestamp& ts, nlohmann::ordered_json& js);
void toJson(const Entry& entry, nlohmann::ordered_json& js);

void fromJson(const nlohmann::json& js, ProcessInfo& pi);
void fromJson(const nlohmann::json& js, WindowInfo& wi);
void fromJson(const nlohmann::json& js, Image& image);
void fromJson(const nlohmann::json& js, Timestamp& ts);
void fromJson(const nlohmann::json& js, Entry& entry);