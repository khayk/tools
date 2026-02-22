#pragma once

#include <core/utils/StopWatch.h>

#include <ostream>
#include <functional>

namespace tools::dups {

class Progress
{
public:
    using DisplayCb = std::function<void(std::ostream&)>;

    explicit Progress(std::ostream* os,
                      std::chrono::milliseconds freq = std::chrono::milliseconds(100))
        : sw_(true)
        , os_(os)
        , freq_ {freq}
    {
    }

    void setFrequency(std::chrono::milliseconds freq)
    {
        freq_ = freq;
    }

    void update(const DisplayCb& cb)
    {
        if (sw_.elapsed() >= freq_)
        {
            if (os_)
            {
                cb(*os_);
                *os_ << '\r';
                os_->flush();
            }
            sw_.restart();
        }
    }

private:
    StopWatch sw_;
    std::ostream* os_ {nullptr};
    std::chrono::milliseconds freq_ {};
};

} // namespace tools::dups
