#pragma once

#include <string>
#include <cstdint>

namespace core::data {

class Unpacker
{
public:
    Unpacker(const Unpacker&) = delete;
    Unpacker& operator=(const Unpacker&) = delete;

    enum class Status : uint8_t
    {
        NeedMore,
        HasMore,
        Ready,
        Invalid //< a frame header declared a length exceeding maxFrameSize
    };

    // Upper bound on a single frame's declared length. A frame header is
    // attacker-controlled, so without a cap a peer can declare an arbitrary
    // size and make us buffer until we run out of memory. 64 MiB comfortably
    // fits the application's largest payloads (e.g. screenshots) while keeping
    // a hostile header from exhausting memory.
    static constexpr size_t DEFAULT_MAX_FRAME_SIZE = 64ULL * 1024 * 1024;

    explicit Unpacker(size_t maxFrameSize = DEFAULT_MAX_FRAME_SIZE);
    ~Unpacker() = default;

    size_t size() const noexcept;
    Status status() const noexcept;

    void put(std::string_view bytes);
    Status get(std::string& buf, size_t maxSize = 64ULL * 1024);

private:
    std::string buffer_;
    size_t off_ {0};
    size_t rem_ {0};
    size_t size_ {0};
    size_t maxFrameSize_;
    Status status_ {Status::NeedMore};
    bool invalid_ {false};

    void readSize() noexcept;
};

} // namespace core::data
