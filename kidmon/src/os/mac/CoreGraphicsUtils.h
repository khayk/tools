#pragma once

// Translation-unit firewall: this header intentionally includes NO project
// geometry headers (Point.h, Rect.h) because MacTypes.h (pulled in by
// CoreGraphics) defines a conflicting global `struct Point`. All CoreGraphics
// calls are isolated inside CoreGraphicsUtils.cpp.

#include <kidmon/common/Enums.h>

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace cg {

struct WindowInfo
{
    uint32_t id {0};
    int32_t pid {0};
    std::string title;
    std::string ownerName;
    int originX {0};
    int originY {0};
    uint32_t width {0};
    uint32_t height {0};
    bool valid {false};
};

// Returns info about the current foreground window, or an invalid WindowInfo.
WindowInfo foregroundWindowInfo();

// Captures window `windowId` into `content` using the given image format.
bool captureWindow(uint32_t windowId, km::ImageFormat format, std::vector<char>& content);

// Returns the executable path of process `pid`.
fs::path processPath(int32_t pid);

} // namespace cg
