#pragma once

#include <kidmon/common/Runnable.h>
#include <kidmon/config/Config.h>
#include <memory>

class KidmonAgent : public Runnable
{
public:
    KidmonAgent(const Config& cfg);
    ~KidmonAgent();

    void run() override;
    void shutdown() noexcept override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
