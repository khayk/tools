#include <gtest/gtest.h>

#include <kidmon/repo/CountingRepository.h>
#include "support/FakeRepository.h"

#include <stdexcept>
#include <string>
#include <vector>

using namespace km;
using km::test::FakeRepository;

namespace {

Entry makeEntry(std::string username)
{
    Entry entry;
    entry.username = std::move(username);
    return entry;
}

} // namespace

TEST(CountingRepositoryTest, CountsStoredAndFailedWrites)
{
    FakeRepository fake;
    CountingRepository repo(fake);

    repo.add(makeEntry("a"));
    fake.failNextAdds(1);
    EXPECT_THROW(repo.add(makeEntry("b")), std::runtime_error);
    repo.add(makeEntry("c"));

    const auto counts = repo.take();
    EXPECT_EQ(counts.stored, 2U);
    EXPECT_EQ(counts.failed, 1U);
    EXPECT_EQ(fake.countFor("a"), 1U);
    EXPECT_EQ(fake.countFor("c"), 1U);
}

TEST(CountingRepositoryTest, AddAllCountsEveryEntry)
{
    FakeRepository fake;
    CountingRepository repo(fake);

    const std::vector<Entry> entries {makeEntry("a"), makeEntry("b")};
    repo.addAll(entries);

    EXPECT_EQ(repo.take().stored, 2U);
    EXPECT_EQ(fake.addAllCalls(), 1U);
}

TEST(CountingRepositoryTest, TakeResetsCounts)
{
    FakeRepository fake;
    CountingRepository repo(fake);

    repo.add(makeEntry("a"));
    std::ignore = repo.take();

    const auto counts = repo.take();
    EXPECT_EQ(counts.stored, 0U);
    EXPECT_EQ(counts.failed, 0U);
}
