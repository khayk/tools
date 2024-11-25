#pragma once

#include <kidmon/common/Runnable.h>

#include <memory>
#include <filesystem>

namespace fs = std::filesystem;

class KidmonServer : public Runnable
{
public:
    struct Config
    {
        Config(fs::path appDataDir);

        fs::path reportsDir;
        std::string authToken;

        std::chrono::milliseconds activityCheckInterval {2000};
        std::chrono::milliseconds peerDropTimeout {activityCheckInterval.count() + 2000};

        uint16_t listenPort {51'097};
        bool spawnAgent {true};
    };

    explicit KidmonServer(const Config& cfg);
    ~KidmonServer();

    void run() override;
    void shutdown() noexcept override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};