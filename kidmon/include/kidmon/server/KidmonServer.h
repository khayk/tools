#pragma once

#include <core/app/Runnable.h>

#include <memory>
#include <filesystem>
#include <chrono>
#include <cstdint>
#include <string>

namespace fs = std::filesystem;

namespace km {

class KidmonServer : public core::Runnable
{
public:
    struct Config
    {
        // Defaults are assigned in the constructor (see KidmonServer.cpp);
        // reportsDir is derived from appDataDir.
        Config(const fs::path& appDataDir);

        fs::path reportsDir;
        std::string authToken;

        std::chrono::milliseconds activityCheckInterval;
        std::chrono::milliseconds peerDropTimeout;

        uint16_t listenPort {};
        bool spawnAgent {};
    };

    explicit KidmonServer(const Config& cfg);
    ~KidmonServer();

    void run() override;
    void shutdown() noexcept override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace km
