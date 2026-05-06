#pragma once

#include <atomic>
#include <string>

class SingleInstanceChecker
{
    std::wstring appName_;
    std::atomic_bool processAlreadyRunning_ {false};
    void* mutex_ {nullptr};

public:
    explicit SingleInstanceChecker(std::wstring_view name);
    ~SingleInstanceChecker();

    bool processAlreadyRunning() const noexcept;
    void report() const;
};
