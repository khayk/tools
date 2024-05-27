#include <kidmon/data/Types.h>
#include <core/utils/File.h>

#include <nlohmann/json.hpp>

void toJson(const ProcessInfo& pi, nlohmann::ordered_json& js)
{
    js["process_path"] = file::path2s(pi.processPath);
    js["sha256"] = pi.sha256;
}

void toJson(const Image& image, nlohmann::ordered_json& js)
{
    js = {
        {"name", image.name},
        {"bytes", image.bytes},
        {"encoded", image.encoded},
    };
}

void toJson(const WindowInfo& wi, nlohmann::ordered_json& js)
{
    js["title"] = wi.title;
    js["rect"] = {
        {"leftTop", {wi.placement.leftTop().x(), wi.placement.leftTop().y()}},
        {"dimensions", {wi.placement.width(), wi.placement.height()}}
    };

    toJson(wi.image, js["image"]);
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

void fromJson(const nlohmann::json& js, ProcessInfo& pi)
{
    pi.processPath.assign(js["process_path"].get<std::string>());
    pi.sha256 = js["sha256"];
}

void fromJson(const nlohmann::json& js, Image& image)
{
    image.name = js["name"];
    image.bytes= js["bytes"];
    image.encoded = js["encoded"];
}

void fromJson(const nlohmann::json& js, WindowInfo& wi)
{
    wi.title = js["title"];

    const auto& jsImg = js["image"];
    fromJson(jsImg, wi.image);

    const auto& jsRect = js["rect"];
    const auto& lt = jsRect["leftTop"];
    const auto& dims = jsRect["dimensions"];

    const Point leftTop(lt.at(0), lt.at(1));
    const Dimensions dimensions(dims.at(0), dims.at(1));

    wi.placement = Rect(leftTop, dimensions);
}

void fromJson(const nlohmann::json& js, TimePoint& tp)
{
    size_t ms = js["since_epoch"];
    const auto& units = js["unit"];

    if (units == "mls")
    {
        tp = TimePoint(std::chrono::milliseconds(ms));
    }
    else
    {
        throw std::logic_error("Unknown time unit");
    }
}

void fromJson(const nlohmann::json& js, Entry& entry)
{
    fromJson(js["process_info"], entry.processInfo);
    fromJson(js["window_info"], entry.windowInfo);
    fromJson(js["timestamp"], entry.timestamp);
}
