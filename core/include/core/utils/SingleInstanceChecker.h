#pragma once

#include <atomic>
#include <cstdint>
#include <string>

namespace core {

class SingleInstanceChecker
{
    std::wstring appName_;
    std::atomic_bool processAlreadyRunning_ {false};
    std::intptr_t mutex_ {0};

public:
    explicit SingleInstanceChecker(std::wstring_view name);
    ~SingleInstanceChecker();

    bool processAlreadyRunning() const noexcept;
    void report() const;
};

} // namespace core