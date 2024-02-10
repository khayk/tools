#include <kidmon/data/Types.h>
#include <core/utils/File.h>

#include <nlohmann/json.hpp>

void toJson(const ProcessInfo& pi, nlohmann::ordered_json& js)
{
    js["process_path"] = file::path2s(pi.processPath);
    js["sha256"] = pi.sha256;
}

void toJson(const WindowInfo& wi, nlohmann::ordered_json& js)
{
    js["title"] = wi.title;
    js["rect"] = {
        {"leftTop", {wi.placement.leftTop().x(), wi.placement.leftTop().y()}},
        {"dimensions", {wi.placement.width(), wi.placement.height()}}
    };
}

void toJson(const TimePoint& tp, nlohmann::ordered_json& js)
{
    using std::chrono::milliseconds;
    using std::chrono::duration_cast;

    js["since_epoch"] = duration_cast<milliseconds>(tp.time_since_epoch()).count();
    js["unit"] = "mls";
}

void toJson(const Entry& entry, nlohmann::ordered_json& js)
{
    toJson(entry.processInfo, js["process_info"]);
    toJson(entry.windowInfo, js["window_info"]);
    toJson(entry.timestamp, js["timestamp"]);
}

