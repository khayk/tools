#include <core/network/data/Unpacker.h>
#include <algorithm>
#include <cstring>

namespace core::data {

Unpacker::Unpacker(size_t maxFrameSize)
    : maxFrameSize_(maxFrameSize)
{
}

void Unpacker::put(std::string_view bytes)
{
    // Once a frame has been rejected the stream is unusable; refuse further
    // input so a hostile peer cannot keep growing our buffer.
    if (invalid_)
    {
        return;
    }

    buffer_ += bytes;
    readSize();

    if (!invalid_ && !bytes.empty())
    {
        status_ = Status::HasMore;
    }
}


size_t Unpacker::size() const noexcept
{
    return size_;
}


Unpacker::Status Unpacker::status() const noexcept
{
    return invalid_ ? Status::Invalid : status_;
}


Unpacker::Status Unpacker::get(std::string& buf, size_t maxSize)
{
    // Stop draining once a frame is rejected; the caller observes the failure
    // through status() and is expected to drop the connection.
    if (invalid_)
    {
        return Status::NeedMore;
    }

    status_ = Status::NeedMore;
    const auto bytes = std::min({rem_, maxSize, buffer_.size() - off_});

    if (bytes == 0)
    {
        return status_;
    }

    buf.append(buffer_.data() + off_, bytes);
    off_ += bytes;
    rem_ -= bytes;
    status_ = Status::HasMore;

    if (rem_ == 0)
    {
        buffer_.erase(0, off_);
        off_ = 0;
        status_ = Status::Ready;
    }
    else if (off_ > maxSize)
    {
        buffer_.erase(0, off_);
        off_ = 0;
    }

    readSize();
    return status_;
}


void Unpacker::readSize() noexcept
{
    if (rem_ == 0 && !invalid_ && buffer_.size() >= sizeof(size_t))
    {
        // memcpy (not a reinterpret_cast) avoids an unaligned/aliasing UB read.
        // The length is host-endian, matching what Packer writes; both ends are
        // on the same machine.
        size_t candidate = 0;
        std::memcpy(&candidate, buffer_.data(), sizeof(candidate));

        if (candidate > maxFrameSize_)
        {
            invalid_ = true;
            return;
        }

        rem_ = candidate;
        off_ += sizeof(size_t);
        size_ = rem_;
    }
}

} // namespace core::data
