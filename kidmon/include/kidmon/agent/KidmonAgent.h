#pragma once

#include <core/app/Runnable.h>

#include <memory>
#include <chrono>
#include <cstdint>
#include <string>

namespace km {

class KidmonAgent : public core::Runnable
{
public:
    struct Config
    {
        // Defaults are assigned in the constructor (see KidmonAgent.cpp).
        Config();

        std::chrono::milliseconds activityCheckInterval;
        std::chrono::milliseconds snapshotInterval;
        // No activity (keyboard/mouse/screen) for this long marks the
        // user as away: the agent then sends heartbeats instead of activity,
        // so idle time is not counted as time-on-task.
        std::chrono::milliseconds idleThreshold;
        bool takeSnapshots {};
        bool calcSha {};
        uint16_t serverPort {};
        std::string authToken;
    };

    KidmonAgent(Config cfg);
    ~KidmonAgent();

    void run() override;
    void shutdown() noexcept override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace km
