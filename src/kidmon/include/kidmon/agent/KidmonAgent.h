#pragma once

#include <kidmon/common/Runnable.h>
#include <memory>
#include <chrono>

class KidmonAgent : public Runnable
{
public:
    struct Config
    {
        std::chrono::milliseconds activityCheckInterval {2000};
        std::chrono::milliseconds snapshotInterval {10'000};
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
