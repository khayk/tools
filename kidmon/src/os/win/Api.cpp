#include <Windows.h>
#include <powrprof.h>

#include "Api.h"
#include "Window.h"
#include "ProcessLauncher.h"

#include <spdlog/spdlog.h>

using namespace km;

namespace km {

ApiPtr ApiFactory::create()
{
    return std::make_unique<ApiImpl>();
}

} // namespace km

WindowPtr ApiImpl::foregroundWindow()
{
    HWND hwnd = GetForegroundWindow();

    if (hwnd != nullptr)
    {
        return std::make_unique<WindowImpl>(hwnd);
    }

    return {};
}

ProcessLauncherPtr ApiImpl::createProcessLauncher()
{
    return std::make_unique<ProcessLauncherImpl>();
}

std::chrono::milliseconds ApiImpl::idleTime()
{
    LASTINPUTINFO lii {};
    lii.cbSize = sizeof(lii);

    if (GetLastInputInfo(&lii) == 0)
    {
        return std::chrono::milliseconds::zero();
    }

    // Both GetTickCount and lii.dwTime are 32-bit millisecond tick counts that
    // wrap roughly every 49.7 days; unsigned subtraction yields the correct
    // elapsed time across a single wrap.
    const DWORD elapsed = GetTickCount() - lii.dwTime;
    return std::chrono::milliseconds(elapsed);
}

bool ApiImpl::displayOn()
{
    // Best-effort proxy: there is no simple synchronous query for the monitor's
    // power state on Windows (it requires listening to GUID_CONSOLE_DISPLAY_STATE
    // power notifications). SystemExecutionState reports the system-wide aggregate
    // of execution-state requirements; the ES_DISPLAY_REQUIRED bit means some app
    // is keeping the display awake (e.g. a media player during playback), which is
    // a reliable subset of "display on". Other on states are not detected here.
    EXECUTION_STATE es = 0;
    if (CallNtPowerInformation(SystemExecutionState, nullptr, 0, &es, sizeof(es)) != 0)
    {
        return false; // not STATUS_SUCCESS
    }

    return (es & ES_DISPLAY_REQUIRED) != 0;
}
