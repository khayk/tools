#pragma once

#include <string>

namespace data {

class Unpacker
{
    Unpacker(const Unpacker&) = delete;
    Unpacker& operator=(const Unpacker&) = delete;

public:
    enum class Status
    {
        NeedMore,
        HasMore,
        Ready
    };

    Unpacker() = default;
    ~Unpacker() = default;

    size_t size() const noexcept;
    Status status() const noexcept;

    void put(std::string_view bytes);
    Status get(std::string& buf, size_t maxSize = 64 * 1024);

private:
    std::string buffer_;
    size_t off_ {0};
    size_t rem_ {0};
    size_t size_ {0};
    Status status_ {Status::NeedMore};

    void readSize() noexcept;
};

} // namespace data
