#pragma once

#include "common/Runnable.h"
#include "config/Config.h"
#include <memory>

class KidMon : public Runnable
{
public:
    KidMon(const Config& cfg);
    ~KidMon();

    void run() override;
    void shutdown() noexcept override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
