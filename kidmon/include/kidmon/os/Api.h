#pragma once

#include <kidmon/os/Window.h>
#include <kidmon/os/ProcessLauncher.h>

#include <chrono>
#include <memory>

namespace km {

class Api
{
public:
    virtual ~Api() = default;

    virtual WindowPtr foregroundWindow() = 0;
    virtual ProcessLauncherPtr createProcessLauncher() = 0;

    /**
     * @brief Time elapsed since the last user input (keyboard or mouse) in the
     *        active session.
     *
     * Used to detect when the user is away so idle time is not counted as
     * activity. Returns 0 on platforms that cannot determine it (the user is
     * then always treated as active).
     */
    virtual std::chrono::milliseconds idleTime() = 0;

    /**
     * @brief Whether some application is currently preventing the display from
     *        sleeping (a power assertion / execution-state request).
     *
     * Media players hold such an assertion while playing, so this is our proxy
     * for "the user is present but passive" (watching a movie / video call with
     * no keyboard or mouse input). When true, the user is treated as active even
     * though @ref idleTime keeps growing, so passive screen time is not lost.
     *
     * Returns false on platforms that cannot determine it.
     */
    virtual bool displaySleepPrevented() = 0;
};

using ApiPtr = std::unique_ptr<Api>;

class ApiFactory
{
public:
    static ApiPtr create();
};

} // namespace km
