#pragma once

#include "common/Runnable.h"
#include "config/Config.h"
#include <memory>

class KidmonService;
class KidmonAgent;

class KidmonService : public Runnable
{
public:
    KidmonService(const Config& cfg);
    ~KidmonService();

    void run() override;
    void shutdown() noexcept override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};