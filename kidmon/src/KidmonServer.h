#pragma once

#include "common/Runnable.h"
#include "config/Config.h"
#include <memory>

class KidmonServer;
class KidmonAgent;

class KidmonServer : public Runnable
{
public:
    KidmonServer(const Config& cfg);
    ~KidmonServer();

    void run() override;
    void shutdown() noexcept override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};