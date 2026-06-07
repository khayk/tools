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
     * @brief Whether the display is currently powered on (awake).
     *
     * Used as a presence signal for "the user is present but passive" (reading,
     * watching a movie, a video call) with no keyboard or mouse input. While the
     * screen is on, the user is treated as active even though @ref idleTime keeps
     * growing, so passive screen time is not lost; once the display sleeps (the
     * user walked away long enough for the OS display-sleep timeout to fire) the
     * time stops counting.
     *
     * Returns true on platforms that cannot determine it (the user is then always
     * treated as present whenever there is a session).
     */
    virtual bool displayOn() = 0;
};

using ApiPtr = std::unique_ptr<Api>;

class ApiFactory
{
public:
    static ApiPtr create();
};

} // namespace km
