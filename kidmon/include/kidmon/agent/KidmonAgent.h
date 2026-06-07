#pragma once

#include <core/app/Runnable.h>
#include <memory>
#include <chrono>

namespace km {

class KidmonAgent : public core::Runnable
{
public:
    struct Config
    {
        std::chrono::milliseconds activityCheckInterval {2000};
        std::chrono::milliseconds snapshotInterval {10'000};
        // No activity (keyboard/mouse/screen) for this long marks the 
        // user as away: the agent then sends heartbeats instead of activity, 
        // so idle time is not counted as time-on-task.
        std::chrono::milliseconds idleThreshold {60'000};
        bool takeSnapshots {false};
        bool calcSha {false};
        uint16_t serverPort {51'097};
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
