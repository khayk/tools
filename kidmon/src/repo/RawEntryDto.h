#pragma once

#include <kidmon/data/Types.h>
#include <kidmon/data/Constants.h>
#include <kidmon/geometry/Rect.h>

#include <glaze/glaze.hpp>

#include <array>
#include <chrono>
#include <cstdint>
#include <string>

namespace km::detail {

// On-disk mirror of the raw entry schema, decoded with glaze straight into
// these reflected structs (glaze's fast path) instead of walking a generic DOM
// by hand. The JSON keys are the shared constants:: -- the same symbols the
// nlohmann writer (toJson) uses -- so a key can never drift between the two.
// The shapes the wire/disk format flattens (rect as two arrays, time as
// epoch-millis, path as a string) are reassembled in toEntry below.
struct ProcDto
{
    std::string path;
    std::string sha256;

    struct glaze
    {
        using T = ProcDto;
        static constexpr auto VALUE =
            glz::object(constants::PROC_PATH, &T::path, constants::PROC_SHA, &T::sha256);
    };
};

struct ImageDto
{
    std::string name;
    std::string bytes;
    bool encoded {false};

    struct glaze
    {
        using T = ImageDto;
        static constexpr auto VALUE = glz::object(constants::WND_IMG_NAME,
                                                  &T::name,
                                                  constants::WND_IMG_BYTES,
                                                  &T::bytes,
                                                  constants::WND_IMG_ENCODED,
                                                  &T::encoded);
    };
};

struct WndDto
{
    std::string title;
    std::array<int32_t, 2> lt {};
    std::array<uint32_t, 2> wh {};
    ImageDto img;

    struct glaze
    {
        using T = WndDto;
        static constexpr auto VALUE = glz::object(constants::WND_TITLE,
                                                  &T::title,
                                                  constants::WND_LEFT_TOP,
                                                  &T::lt,
                                                  constants::WND_DIMENSIONS,
                                                  &T::wh,
                                                  constants::WND_IMG,
                                                  &T::img);
    };
};

struct TimestampDto
{
    int64_t when {0};
    int64_t dur {0};

    struct glaze
    {
        using T = TimestampDto;
        static constexpr auto VALUE = glz::object(constants::TIMESTAMP_WHEN,
                                                  &T::when,
                                                  constants::TIMESTAMP_DUR,
                                                  &T::dur);
    };
};

struct EntryDto
{
    ProcDto proc;
    WndDto wnd;
    TimestampDto ts;

    struct glaze
    {
        using T = EntryDto;
        static constexpr auto VALUE = glz::object(constants::PROC_INFO,
                                                  &T::proc,
                                                  constants::WND_INFO,
                                                  &T::wnd,
                                                  constants::TIMESTAMP,
                                                  &T::ts);
    };
};

inline void toEntry(const EntryDto& dto, Entry& entry)
{
    entry.processInfo.processPath = dto.proc.path;
    entry.processInfo.sha256 = dto.proc.sha256;

    entry.windowInfo.title = dto.wnd.title;
    entry.windowInfo.placement = Rect(Point(dto.wnd.lt[0], dto.wnd.lt[1]),
                                      Dimensions(dto.wnd.wh[0], dto.wnd.wh[1]));
    entry.windowInfo.image.name = dto.wnd.img.name;
    entry.windowInfo.image.bytes = dto.wnd.img.bytes;
    entry.windowInfo.image.encoded = dto.wnd.img.encoded;

    entry.timestamp.capture = TimePoint(std::chrono::milliseconds(dto.ts.when));
    entry.timestamp.duration = std::chrono::milliseconds(dto.ts.dur);
}

} // namespace km::detail
