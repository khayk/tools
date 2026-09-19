#pragma once

#include <core/app/Runnable.h>

#include <memory>
#include <filesystem>
#include <chrono>
#include <cstdint>
#include <string>

namespace fs = std::filesystem;

namespace km {

// Which repository backend(s) the server persists entries to. Only the
// backend(s) selected here are ever constructed -- e.g. FileSystem does not
// touch dbFile at all -- so a caller that only needs one (a test, most
// notably) doesn't pay for the other.
enum class RepositoryBackend : std::uint8_t
{
    FileSystem, // legacy on-disk layout only
    Sqlite,     // sqlite db only
    Both,       // write to both; FileSystem remains the query source
};

class KidmonServer : public core::Runnable
{
public:
    struct Config
    {
        // Defaults are assigned in the constructor (see KidmonServer.cpp);
        // reportsDir is derived from appDataDir.
        Config(const fs::path& appDataDir);

        fs::path reportsDir;
        fs::path dbFile;
        std::string authToken;

        std::chrono::milliseconds activityCheckInterval;
        std::chrono::milliseconds peerDropTimeout;

        uint16_t listenPort {};
        bool spawnAgent {};

        // Sqlite is new and unproven, so defaulting to Both keeps a one-line
        // rollback available if it misbehaves in the field: switch this back
        // to FileSystem (no other code change needed) to stop touching it.
        RepositoryBackend repoBackend {RepositoryBackend::Both};
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
