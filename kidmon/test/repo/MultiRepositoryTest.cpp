#include <gtest/gtest.h>

#include <kidmon/repo/MultiRepository.h>
#include "support/FakeRepository.h"

#include <functional>
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

TEST(MultiRepositoryTest, EmptyRepositoryListThrows)
{
    EXPECT_THROW(MultiRepository(std::vector<std::reference_wrapper<IRepository>> {}),
                 std::invalid_argument);
}

TEST(MultiRepositoryTest, AddWritesToEveryRepository)
{
    FakeRepository primary;
    FakeRepository secondary;
    MultiRepository repo(primary, secondary);

    repo.add(makeEntry("john"));

    EXPECT_EQ(primary.countFor("john"), 1U);
    EXPECT_EQ(secondary.countFor("john"), 1U);
}

TEST(MultiRepositoryTest, QueriesAreAnsweredByThePrimaryOnly)
{
    FakeRepository primary;
    FakeRepository secondary;
    MultiRepository repo(primary, secondary);

    // Written straight to the fakes (bypassing MultiRepository::add) so the
    // two repositories can be made to disagree, proving queries never touch
    // the secondary.
    secondary.add(makeEntry("only-in-secondary"));

    int usersSeen = 0;
    repo.queryUsers([&usersSeen](const std::string&) {
        ++usersSeen;
        return true;
    });
    EXPECT_EQ(usersSeen, 0);

    primary.add(makeEntry("only-in-primary"));
    repo.queryUsers([&usersSeen](const std::string& username) {
        EXPECT_EQ(username, "only-in-primary");
        ++usersSeen;
        return true;
    });
    EXPECT_EQ(usersSeen, 1);
}

TEST(MultiRepositoryTest, SecondaryFailureIsSwallowedAndDoesNotBlockPrimary)
{
    FakeRepository primary;
    FakeRepository secondary;
    secondary.failNextAdds(1);
    MultiRepository repo(primary, secondary);

    EXPECT_NO_THROW(repo.add(makeEntry("john")));

    EXPECT_EQ(primary.countFor("john"), 1U);
    EXPECT_EQ(secondary.addCalls(), 1U);
    EXPECT_EQ(secondary.countFor("john"),
              0U); // the simulated failure prevented storage
}

TEST(MultiRepositoryTest, PrimaryFailurePropagatesButSecondaryStillReceivesTheWrite)
{
    FakeRepository primary;
    FakeRepository secondary;
    primary.failNextAdds(1);
    MultiRepository repo(primary, secondary);

    EXPECT_THROW(repo.add(makeEntry("john")), std::runtime_error);

    EXPECT_EQ(primary.countFor("john"), 0U);
    EXPECT_EQ(secondary.countFor("john"), 1U);
}

TEST(MultiRepositoryTest, SupportsMoreThanTwoRepositories)
{
    FakeRepository a;
    FakeRepository b;
    FakeRepository c;
    MultiRepository repo(std::vector<std::reference_wrapper<IRepository>> {a, b, c});

    repo.add(makeEntry("john"));

    EXPECT_EQ(a.countFor("john"), 1U);
    EXPECT_EQ(b.countFor("john"), 1U);
    EXPECT_EQ(c.countFor("john"), 1U);
}
