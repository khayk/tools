#include <gtest/gtest.h>
#include <core/network/data/Packer.h>
#include <core/network/data/Unpacker.h>
#include <core/network/data/StringSource.h>

#include <array>
#include <cstring>

using namespace core::data;

namespace {

// Build a raw wire frame: an 8-byte host-endian length prefix (as Packer
// writes) followed by the body. Lets a test declare an arbitrary length
// independently of the body actually supplied.
std::string frame(size_t declaredSize, std::string_view body)
{
    std::string out(sizeof(size_t), '\0');
    std::memcpy(out.data(), &declaredSize, sizeof(declaredSize));
    out.append(body);
    return out;
}

class EmulateLargeSource : public ISource
{
    size_t size_;
    size_t rem_;

public:
    explicit EmulateLargeSource(size_t size)
        : size_(size)
        , rem_(size)
    {
    }

    size_t size() const noexcept override
    {
        return size_;
    }

    size_t get(std::string& buf, size_t maxSize) override
    {
        const auto bytes = std::min(rem_, maxSize);
        buf.append(bytes, 'a');
        rem_ -= bytes;

        return bytes;
    }
};

std::string pack(std::string_view data)
{
    StringSource src(data);
    Packer packer(src);

    std::string packed;
    while (packer.get(packed));

    return packed;
}

} // namespace


TEST(UnpackerTest, NoData)
{
    Unpacker unpacker;
    std::string buf;

    EXPECT_EQ(unpacker.size(), 0);
    EXPECT_EQ(unpacker.status(), Unpacker::Status::NeedMore);
    EXPECT_EQ(unpacker.get(buf), Unpacker::Status::NeedMore);
    EXPECT_EQ(unpacker.status(), Unpacker::Status::NeedMore);
}

TEST(UnpackerTest, UnpackData)
{
    constexpr std::string_view data("1234567");
    Unpacker unpacker;

    unpacker.put(pack(data));
    EXPECT_EQ(data.size(), unpacker.size());

    std::string buf;
    // current message is fully retreived
    EXPECT_EQ(unpacker.get(buf), Unpacker::Status::Ready);
    EXPECT_EQ(buf, data);

    // no more data available
    EXPECT_EQ(unpacker.get(buf), Unpacker::Status::NeedMore);
}


TEST(UnpackerTest, UnpackChunkedData)
{
    constexpr std::string_view data("payload");
    Unpacker unpacker;

    // Data size should be available right after the `put`
    unpacker.put(pack(data));
    EXPECT_EQ(unpacker.size(), data.size());

    std::string buf;
    while (unpacker.get(buf, 3) != Unpacker::Status::Ready)
    {
        EXPECT_EQ(unpacker.size(), data.size());
        ASSERT_NE(unpacker.status(), Unpacker::Status::NeedMore);
    }
    EXPECT_EQ(buf, data);
}


namespace {

void unpackIncompleteData(const std::string_view data, size_t chunkSize)
{
    Unpacker unpacker;

    const auto packed = pack(data);
    std::string_view sv(packed);
    sv.remove_suffix(4);
    unpacker.put(sv);
    EXPECT_EQ(unpacker.size(), data.size());

    std::string buf;
    while (unpacker.get(buf, chunkSize) == Unpacker::Status::HasMore)
    {
        EXPECT_EQ(unpacker.size(), data.size());
    }

    EXPECT_EQ(unpacker.status(), Unpacker::Status::NeedMore);
    std::string_view sv2(packed);
    sv2.remove_prefix(sv.size());

    unpacker.put(sv2);
    EXPECT_EQ(unpacker.get(buf), Unpacker::Status::Ready);
    EXPECT_EQ(buf, data);
}

} // namespace


TEST(UnpackerTest, UnpackIncompleteData_SmallChunks)
{
    constexpr std::string_view data("payload");
    unpackIncompleteData(data, data.size() / 2);
}


TEST(UnpackerTest, UnpackIncompleteData_BigChunks)
{
    constexpr std::string_view data("payload");
    unpackIncompleteData(data, 3 * data.size() / 2);
    unpackIncompleteData(data, 4 * data.size());
}


TEST(UnpackerTest, UnpackBatchData)
{
    std::array quotes = {"Winston Churchill quotes",
                         "I am easily satisfied with the very best.",
                         "If you're going through hell, keep going.",
                         "The price of greatness is responsibility."};

    Unpacker unpacker;
    std::string bytes;
    std::string unpacked;

    for (const auto& quote : quotes)
    {
        bytes = pack(quote);
        unpacker.put(bytes);

        unpacked.clear();
        EXPECT_EQ(unpacker.get(unpacked), Unpacker::Status::Ready);
        EXPECT_EQ(unpacked, quote);
    }
}


TEST(UnpackerTest, UnpackData_1Gb)
{
    EmulateLargeSource ls(1024ULL * 1024 * 1024);
    Packer packer(ls);
    // Raise the frame cap above the 1 GiB payload this test streams.
    Unpacker unpacker(2ULL * 1024 * 1024 * 1024);

    std::string buf;
    std::string out;
    bool first = true;

    while (packer.get(buf))
    {
        unpacker.put(buf);
        unpacker.get(out);

        if (!first)
        {
            ASSERT_EQ(buf, out);
        }

        first = false;
        buf.clear();
        out.clear();
    }
}


TEST(UnpackerTest, RejectsOversizedFrameHeader)
{
    Unpacker unpacker(1024); // 1 KiB cap
    std::string buf;

    // Header declares 4 KiB, exceeding the cap. No body is even supplied: the
    // header alone must be rejected, before any large buffer is accumulated.
    unpacker.put(frame(4096, ""));
    EXPECT_EQ(unpacker.status(), Unpacker::Status::Invalid);
    EXPECT_EQ(unpacker.get(buf), Unpacker::Status::NeedMore);
    EXPECT_TRUE(buf.empty());

    // Further input is ignored — no unbounded growth after rejection.
    unpacker.put(std::string(100'000, 'x'));
    EXPECT_EQ(unpacker.status(), Unpacker::Status::Invalid);
    EXPECT_EQ(unpacker.get(buf), Unpacker::Status::NeedMore);
    EXPECT_TRUE(buf.empty());
}


TEST(UnpackerTest, AcceptsFrameAtMaxSize)
{
    Unpacker unpacker(8); // cap exactly at the body size
    std::string buf;

    unpacker.put(frame(8, "ABCDEFGH"));
    EXPECT_EQ(unpacker.status(), Unpacker::Status::HasMore);
    EXPECT_EQ(unpacker.size(), 8U);
    EXPECT_EQ(unpacker.get(buf), Unpacker::Status::Ready);
    EXPECT_EQ(buf, "ABCDEFGH");
}


TEST(UnpackerTest, RejectsFrameOneOverMaxSize)
{
    Unpacker unpacker(8);
    std::string buf;

    unpacker.put(frame(9, "ABCDEFGHI"));
    EXPECT_EQ(unpacker.status(), Unpacker::Status::Invalid);
    EXPECT_EQ(unpacker.get(buf), Unpacker::Status::NeedMore);
    EXPECT_TRUE(buf.empty());
}
