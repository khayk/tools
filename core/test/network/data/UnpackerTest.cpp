#include <gtest/gtest.h>
#include "network/data/Packer.h"
#include "network/data/Unpacker.h"
#include "network/data/StringSource.h"

#include <array>

using namespace data;

namespace {

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
    while (packer.get(packed))
        ;

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


TEST(UnpackerTest, UnpackIncompleteData)
{
    constexpr std::string_view data("payload");
    Unpacker unpacker;

    const auto packed = pack(data);
    std::string_view sv(packed);
    sv.remove_suffix(4);
    unpacker.put(sv);
    EXPECT_EQ(unpacker.size(), data.size());

    std::string buf;
    while (unpacker.get(buf, 3) == Unpacker::Status::HasMore)
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
    EmulateLargeSource ls(1024ull * 1024 * 1024);
    Packer packer(ls);
    Unpacker unpacker;

    std::string packed;
    std::string unpacked;
    bool first = true;

    while (packer.get(packed))
    {
        unpacker.put(packed);
        unpacker.get(unpacked);

        if (!first)
        {
            EXPECT_EQ(packed, unpacked);
            first = false;
        }

        packed.clear();
        unpacked.clear();
    }
}
