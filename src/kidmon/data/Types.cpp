#include <kidmon/data/Types.h>
#include <core/utils/File.h>

#include <nlohmann/json.hpp>

namespace constants {
    constexpr std::string_view PROC_INFO = "proc";
    constexpr std::string_view PROC_PATH = "path";
    constexpr std::string_view PROC_SHA = "sha256";

    constexpr std::string_view WND_INFO = "wnd";
    constexpr std::string_view WND_TITLE = "title";
    constexpr std::string_view WND_LEFT_TOP = "lt";
    constexpr std::string_view WND_DIMENSIONS = "wh";
    constexpr std::string_view WND_IMG = "img";

    constexpr std::string_view WND_IMG_NAME = "name";
    constexpr std::string_view WND_IMG_BYTES = "bytes";
    constexpr std::string_view WND_IMG_ENCODED = "encoded";

    constexpr std::string_view TIMESTAMP = "ts";
    constexpr std::string_view TIMESTAMP_WHEN = "when";
    constexpr std::string_view TIMESTAMP_DUR = "dur";
} // namespace constants

void toJson(const ProcessInfo& pi, nlohmann::ordered_json& js)
{
    js[constants::PROC_PATH] = file::path2s(pi.processPath);
    js[constants::PROC_SHA] = pi.sha256;
}

void toJson(const Image& image, nlohmann::ordered_json& js)
{
    js = {
        {constants::WND_IMG_NAME, image.name},
        {constants::WND_IMG_BYTES, image.bytes},
        {constants::WND_IMG_ENCODED, image.encoded},
    };
}

void toJson(const WindowInfo& wi, nlohmann::ordered_json& js)
{
    js[constants::WND_TITLE] = wi.title;
    js[constants::WND_LEFT_TOP] = {wi.placement.leftTop().x(), wi.placement.leftTop().y()};
    js[constants::WND_DIMENSIONS] = {wi.placement.width(), wi.placement.height()};

    toJson(wi.image, js[constants::WND_IMG]);
}

void toJson(const Timestamp& ts, nlohmann::ordered_json& js)
{
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;

    js[constants::TIMESTAMP_WHEN] =
        duration_cast<milliseconds>(ts.capture.time_since_epoch()).count();
    js[constants::TIMESTAMP_DUR] = ts.duration.count();
}

void toJson(const Entry& entry, nlohmann::ordered_json& js)
{
    toJson(entry.processInfo, js[constants::PROC_INFO]);
    toJson(entry.windowInfo, js[constants::WND_INFO]);
    toJson(entry.timestamp, js[constants::TIMESTAMP]);
}

void fromJson(const nlohmann::json& js, ProcessInfo& pi)
{
    pi.processPath.assign(js[constants::PROC_PATH].get<std::string>());
    pi.sha256 = js[constants::PROC_SHA];
}

void fromJson(const nlohmann::json& js, Image& image)
{
    image.name = js[constants::WND_IMG_NAME];
    image.bytes = js[constants::WND_IMG_BYTES];
    image.encoded = js[constants::WND_IMG_ENCODED];
}

void fromJson(const nlohmann::json& js, WindowInfo& wi)
{
    wi.title = js[constants::WND_TITLE];

    const auto& jsImg = js[constants::WND_IMG];
    fromJson(jsImg, wi.image);

    const auto& lt = js[constants::WND_LEFT_TOP];
    const auto& dims = js[constants::WND_DIMENSIONS];

    const Point leftTop(lt.at(0), lt.at(1));
    const Dimensions dimensions(dims.at(0), dims.at(1));

    wi.placement = Rect(leftTop, dimensions);
}

void fromJson(const nlohmann::json& js, Timestamp& ts)
{
    ts.capture = TimePoint(std::chrono::milliseconds(js[constants::TIMESTAMP_WHEN]));
    ts.duration = std::chrono::milliseconds(js[constants::TIMESTAMP_DUR]);
}

void fromJson(const nlohmann::json& js, Entry& entry)
{
    fromJson(js[constants::PROC_INFO], entry.processInfo);
    fromJson(js[constants::WND_INFO], entry.windowInfo);
    fromJson(js[constants::TIMESTAMP], entry.timestamp);
}
