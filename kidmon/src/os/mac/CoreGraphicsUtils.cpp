// CoreGraphics MUST be included first in this TU. MacTypes.h (transitively
// included by CoreGraphics) defines a global `struct Point` that conflicts
// with the project's `class Point`. By never including project geometry
// headers here, the conflict is avoided.
#include <CoreGraphics/CoreGraphics.h>

#include "CoreGraphicsUtils.h"

#include <libproc.h>
#include <spdlog/spdlog.h>

#include <array>

namespace {

std::string cfStringToString(CFStringRef ref)
{
    if (!ref)
    {
        return {};
    }
    const char* cstr = CFStringGetCStringPtr(ref, kCFStringEncodingUTF8);
    if (cstr)
    {
        return cstr;
    }
    const CFIndex maxLen =
        CFStringGetMaximumSizeForEncoding(CFStringGetLength(ref),
                                          kCFStringEncodingUTF8) +
        1;
    std::array<char, 1024> buf {};
    if (CFStringGetCString(ref, buf.data(), maxLen, kCFStringEncodingUTF8))
    {
        return buf.data();
    }
    return {};
}

} // namespace

namespace cg {

WindowInfo foregroundWindowInfo()
{
    CFArrayRef rawList =
        CGWindowListCopyWindowInfo(kCGWindowListOptionOnScreenOnly |
                                       kCGWindowListExcludeDesktopElements,
                                   kCGNullWindowID);
    if (!rawList)
    {
        return {};
    }
    std::unique_ptr<std::remove_pointer_t<CFArrayRef>, decltype(&CFRelease)>
        list(rawList, CFRelease);

    const CFIndex count = CFArrayGetCount(rawList);
    for (CFIndex i = 0; i < count; ++i)
    {
        const auto* info =
            static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(rawList, i));

        // CGWindowList is ordered front-to-back. Skip non-normal layers
        // (menu bar, dock, overlays live at non-zero layers).
        int32_t layer = 0;
        if (const auto* n = static_cast<CFNumberRef>(
                CFDictionaryGetValue(info, kCGWindowLayer)))
        {
            CFNumberGetValue(n, kCFNumberSInt32Type, &layer);
        }
        if (layer != 0)
        {
            continue;
        }

        const auto* pidNum = static_cast<CFNumberRef>(
            CFDictionaryGetValue(info, kCGWindowOwnerPID));
        if (!pidNum)
        {
            continue;
        }
        int32_t pid = 0;
        CFNumberGetValue(pidNum, kCFNumberSInt32Type, &pid);

        uint32_t windowId = kCGNullWindowID;
        if (const auto* n = static_cast<CFNumberRef>(
                CFDictionaryGetValue(info, kCGWindowNumber)))
        {
            CFNumberGetValue(n, kCFNumberSInt32Type, &windowId);
        }

        std::string title;
        if (const auto* t = static_cast<CFStringRef>(
                CFDictionaryGetValue(info, kCGWindowName)))
        {
            title = cfStringToString(t);
        }

        std::string ownerName;
        if (const auto* o = static_cast<CFStringRef>(
                CFDictionaryGetValue(info, kCGWindowOwnerName)))
        {
            ownerName = cfStringToString(o);
        }

        CGRect cgRect = CGRectZero;
        if (const auto* bd = static_cast<CFDictionaryRef>(
                CFDictionaryGetValue(info, kCGWindowBounds)))
        {
            CGRectMakeWithDictionaryRepresentation(bd, &cgRect);
        }

        return {
            .id        = windowId,
            .pid       = pid,
            .title     = std::move(title),
            .ownerName = std::move(ownerName),
            .originX   = static_cast<int>(cgRect.origin.x),
            .originY   = static_cast<int>(cgRect.origin.y),
            .width     = static_cast<uint32_t>(cgRect.size.width),
            .height    = static_cast<uint32_t>(cgRect.size.height),
            .valid     = true,
        };
    }
    return {};
}

bool captureWindow([[maybe_unused]] uint32_t windowId,
                   [[maybe_unused]] km::ImageFormat format,
                   [[maybe_unused]] std::vector<char>& content)
{
    // Per-window screen capture was removed from CoreGraphics in macOS 15.
    // The replacement (ScreenCaptureKit) is Objective-C only. Implementing
    // this would require a .mm compilation unit.
    spdlog::warn("captureWindow: not implemented on macOS (requires ScreenCaptureKit)");
    return false;
}

fs::path processPath(int32_t pid)
{
    std::array<char, PROC_PIDPATHINFO_MAXSIZE> buf {};
    if (proc_pidpath(static_cast<pid_t>(pid), buf.data(), buf.size()) > 0)
    {
        return {buf.data()};
    }
    return {};
}

} // namespace cg
