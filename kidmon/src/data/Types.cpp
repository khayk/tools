#include <kidmon/data/Types.h>
#include <core/utils/File.h>

#include <nlohmann/json.hpp>

#define PROC_INFO "proc"
#define PROC_PATH "path"
#define PROC_SHA "sha256"

#define WND_INFO "wnd"
#define WND_TITLE "title"
#define WND_LEFT_TOP "lt"
#define WND_DIMENSIONS "wh"
#define WND_IMG "img"

#define WND_IMG_NAME "name"
#define WND_IMG_BYTES "bytes"
#define WND_IMG_ENCODED "encoded"

#define TIMESTAMP "ts"
#define TIMESTAMP_WHEN "when"
#define TIMESTAMP_DUR "dur"

void toJson(const ProcessInfo& pi, nlohmann::ordered_json& js)
{
    js[PROC_PATH] = file::path2s(pi.processPath);
    js[PROC_SHA] = pi.sha256;
}

void toJson(const Image& image, nlohmann::ordered_json& js)
{
    js = {
        {WND_IMG_NAME, image.name},
        {WND_IMG_BYTES, image.bytes},
        {WND_IMG_ENCODED, image.encoded},
    };
}

void toJson(const WindowInfo& wi, nlohmann::ordered_json& js)
{
    js[WND_TITLE] = wi.title;
    js[WND_LEFT_TOP] = {wi.placement.leftTop().x(), wi.placement.leftTop().y()};
    js[WND_DIMENSIONS] = {wi.placement.width(), wi.placement.height()};

    toJson(wi.image, js[WND_IMG]);
}

void toJson(const Timestamp& ts, nlohmann::ordered_json& js)
{
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;

    js[TIMESTAMP_WHEN] =
        duration_cast<milliseconds>(ts.capture.time_since_epoch()).count();
    js[TIMESTAMP_DUR] = ts.duration.count();
}

void toJson(const Entry& entry, nlohmann::ordered_json& js)
{
    toJson(entry.processInfo, js[PROC_INFO]);
    toJson(entry.windowInfo, js[WND_INFO]);
    toJson(entry.timestamp, js[TIMESTAMP]);
}

void fromJson(const nlohmann::json& js, ProcessInfo& pi)
{
    pi.processPath.assign(js[PROC_PATH].get<std::string>());
    pi.sha256 = js[PROC_SHA];
}

void fromJson(const nlohmann::json& js, Image& image)
{
    image.name = js[WND_IMG_NAME];
    image.bytes = js[WND_IMG_BYTES];
    image.encoded = js[WND_IMG_ENCODED];
}

void fromJson(const nlohmann::json& js, WindowInfo& wi)
{
    wi.title = js[WND_TITLE];

    const auto& jsImg = js[WND_IMG];
    fromJson(jsImg, wi.image);

    const auto& lt = js[WND_LEFT_TOP];
    const auto& dims = js[WND_DIMENSIONS];

    const Point leftTop(lt.at(0), lt.at(1));
    const Dimensions dimensions(dims.at(0), dims.at(1));

    wi.placement = Rect(leftTop, dimensions);
}

void fromJson(const nlohmann::json& js, Timestamp& ts)
{
    ts.capture = TimePoint(std::chrono::milliseconds(js[TIMESTAMP_WHEN]));
    ts.duration = std::chrono::milliseconds(js[TIMESTAMP_DUR]);
}

void fromJson(const nlohmann::json& js, Entry& entry)
{
    fromJson(js[PROC_INFO], entry.processInfo);
    fromJson(js[WND_INFO], entry.windowInfo);
    fromJson(js[TIMESTAMP], entry.timestamp);
}
