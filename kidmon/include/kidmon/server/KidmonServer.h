#pragma once

#include <kidmon/common/Runnable.h>
#include <kidmon/config/Config.h>

#include <memory>

class KidmonServer;
class KidmonAgent;

class KidmonServer : public Runnable
{
public:
    explicit KidmonServer(const Config& cfg);
    ~KidmonServer();

    void run() override;
    void shutdown() noexcept override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};