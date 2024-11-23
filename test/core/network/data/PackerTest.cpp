#include <gtest/gtest.h>

#include <core/network/data/Packer.h>
#include <core/network/data/StringSource.h>

using namespace data;

TEST(PackerTest, PackData)
{
    constexpr std::string_view data("hello");
    StringSource src(data);
    Packer packer(src);

    std::string bytes;
    EXPECT_EQ(data.size(), packer.get(bytes));
    EXPECT_EQ(data.size() + sizeof(size_t), bytes.size());

    std::string_view sv(bytes);
    size_t size = *reinterpret_cast<const size_t*>(sv.data());
    EXPECT_EQ(size, data.size());

    sv.remove_prefix(sizeof(size_t));
    EXPECT_EQ(sv, data);

    // Consecutive calls has nothing to return
    EXPECT_EQ(0, packer.get(bytes));
}


TEST(PackerTest, PackDataChunked)
{
    constexpr std::string_view data("payload");
    StringSource src(data);
    Packer packer(src);

    const size_t chunkSize = 2;
    std::string packed;
    size_t readBytes = 0;
    size_t readTotal = 0;

    while ((readBytes = packer.get(packed, chunkSize)) > 0)
    {
        readTotal += readBytes;
        EXPECT_LE(readBytes, chunkSize);
        EXPECT_EQ(packed.size(), sizeof(size_t) + readTotal);
    }
}
