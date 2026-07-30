#include "AppPath.h"

#include <core/utils/FmtExt.h>
#include <spdlog/spdlog.h>

// NOTE: kept in its own translation unit. CoreServices drags in the Carbon
// MacTypes.h, whose global `Rect` would collide with km::Rect everywhere else.
#include <CoreServices/CoreServices.h>

#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

namespace km {

namespace {

// Chrome names the holding directory "<bundle id>.code_sign_clone", which is
// where the bundle identifier is recovered from, and the bundle inside it
// "<app name>.app.bundle".
constexpr std::string_view CLONE_DIR_SUFFIX = ".code_sign_clone";
constexpr std::string_view CLONE_BUNDLE_SUFFIX = ".app.bundle";

/**
 * @brief Minimal owning handle for a Core Foundation object.
 */
template <typename Ref>
class CFRef
{
    Ref ref_ {nullptr};

public:
    CFRef() = default;
    ~CFRef()
    {
        if (ref_)
        {
            CFRelease(ref_);
        }
    }

    CFRef(const CFRef&) = delete;
    CFRef& operator=(const CFRef&) = delete;

    explicit CFRef(Ref ref) noexcept
        : ref_(ref)
    {
    }

    [[nodiscard]] Ref get() const noexcept
    {
        return ref_;
    }
    explicit operator bool() const noexcept
    {
        return ref_ != nullptr;
    }
};

std::string toStdString(CFStringRef str)
{
    if (!str)
    {
        return {};
    }

    if (const char* fast = CFStringGetCStringPtr(str, kCFStringEncodingUTF8))
    {
        return fast;
    }

    const CFIndex length = CFStringGetLength(str);
    const CFIndex capacity =
        CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;

    std::string out(static_cast<size_t>(capacity), '\0');
    if (!CFStringGetCString(str, out.data(), capacity, kCFStringEncodingUTF8))
    {
        return {};
    }
    out.resize(std::char_traits<char>::length(out.c_str()));

    return out;
}

/**
 * @brief Where LaunchServices thinks the given bundle identifier is installed.
 *
 * @param bundleId The identifier to look up, e.g. "com.google.Chrome"
 * @param tail     Path of the executable relative to the bundle root, used to
 *                 pick between several installs of the same application
 * @return Full path to the executable, or empty if no install provides it
 */
fs::path locateExecutable(const std::string& bundleId, const fs::path& tail)
{
    const CFRef<CFStringRef> id(
        CFStringCreateWithBytes(nullptr,
                                reinterpret_cast<const UInt8*>(bundleId.data()),
                                static_cast<CFIndex>(bundleId.size()),
                                kCFStringEncodingUTF8,
                                FALSE));

    if (!id)
    {
        return {};
    }

    const CFRef<CFArrayRef> urls(
        LSCopyApplicationURLsForBundleIdentifier(id.get(), nullptr));

    if (!urls)
    {
        return {};
    }

    // Since macOS 10.15 the best match comes first, so the first candidate that
    // actually holds the executable wins.
    for (CFIndex i = 0, n = CFArrayGetCount(urls.get()); i < n; ++i)
    {
        const auto* url = static_cast<CFURLRef>(CFArrayGetValueAtIndex(urls.get(), i));

        const CFRef<CFStringRef> path(
            CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle));

        fs::path candidate = fs::path(toStdString(path.get())) / tail;

        std::error_code ec;
        if (fs::exists(candidate, ec))
        {
            return candidate;
        }
    }

    return {};
}

/**
 * @brief locateExecutable, remembering what it answered.
 *
 * Every sample of a foreground window goes through here, and for as long as the
 * user stays in an updated-but-not-yet-restarted Chrome that means every sample
 * hits a code sign clone. The LaunchServices database is not worth querying at
 * that rate; where an application is installed does not change while it runs.
 */
fs::path cachedExecutable(const std::string& bundleId, const fs::path& tail)
{
    static std::mutex mutex;
    static std::unordered_map<std::string, fs::path> cache;

    const std::lock_guard lock(mutex);
    const auto it = cache.find(bundleId);
    if (it != cache.end())
    {
        return it->second;
    }

    auto resolved = locateExecutable(bundleId, tail);
    if (resolved.empty())
    {
        // Also remembered: an application LaunchServices does not know about is
        // not going to become known while it keeps running.
        spdlog::warn("Unable to locate the installed bundle of '{}'", bundleId);
    }
    else
    {
        spdlog::info("Resolved '{}' code sign clone to '{}'", bundleId, resolved);
    }

    return cache.emplace(bundleId, std::move(resolved)).first->second;
}

} // namespace

fs::path resolveRealAppPath(const fs::path& execPath)
{
    auto it = execPath.begin();
    const auto end = execPath.end();

    for (; it != end; ++it)
    {
        if (it->native().size() > CLONE_DIR_SUFFIX.size() &&
            it->native().ends_with(CLONE_DIR_SUFFIX))
        {
            break;
        }
    }

    if (it == end)
    {
        return execPath; // not a code sign clone, nothing to do
    }

    const std::string bundleId =
        it->native().substr(0, it->native().size() - CLONE_DIR_SUFFIX.size());

    // Everything below "<app name>.app.bundle" mirrors the real bundle, so it
    // carries over untouched. Searching for that component rather than counting
    // directories keeps this working if the layout in between ever changes.
    for (; it != end; ++it)
    {
        if (it->native().ends_with(CLONE_BUNDLE_SUFFIX))
        {
            break;
        }
    }

    if (it == end)
    {
        spdlog::debug("No '{}' component in '{}'", CLONE_BUNDLE_SUFFIX, execPath);
        return execPath;
    }

    fs::path tail;
    for (++it; it != end; ++it)
    {
        tail /= *it;
    }

    if (tail.empty())
    {
        return execPath;
    }

    auto resolved = cachedExecutable(bundleId, tail);

    return resolved.empty() ? execPath : resolved;
}

} // namespace km
