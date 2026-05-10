#include "Api.h"
#include "Window.h"
#include "ProcessLauncher.h"

#include <kidmon/geometry/Dimensions.h>
#include <kidmon/geometry/Point.h>

#include <spdlog/spdlog.h>
#include <CoreGraphics/CoreGraphics.h>

#include <vector>

using namespace km;

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
                                          kCFStringEncodingUTF8) + 1;
    std::vector<char> buf(static_cast<size_t>(maxLen));
    if (CFStringGetCString(ref, buf.data(), maxLen, kCFStringEncodingUTF8))
    {
        return buf.data();
    }
    return {};
}

} // namespace

namespace km {

ApiPtr ApiFactory::create()
{
    return std::make_unique<ApiImpl>();
}

} // namespace km

WindowPtr ApiImpl::foregroundWindow()
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

        // Qualify explicitly: unqualified Point/Rect would be ambiguous between
        // ::Point/::Rect (MacTypes.h) and km::Point/km::Rect (via using namespace km).
        km::Point origin(static_cast<int>(cgRect.origin.x),
                         static_cast<int>(cgRect.origin.y));
        km::Dimensions dims(static_cast<uint32_t>(cgRect.size.width),
                            static_cast<uint32_t>(cgRect.size.height));

        return std::make_unique<WindowImpl>(windowId,
                                            static_cast<pid_t>(pid),
                                            std::move(title),
                                            std::move(ownerName),
                                            km::Rect(origin, dims));
    }

    return {};
}

ProcessLauncherPtr ApiImpl::createProcessLauncher()
{
    return std::make_unique<ProcessLauncherImpl>();
}
