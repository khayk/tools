#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <duplicates/Menu.h>

using ::testing::_;
using ::testing::InSequence;

namespace tools::dups {

class MockMenuActions
{
public:
    MOCK_METHOD(void, optionSelected, (std::string option), ());
    MOCK_METHOD(void, invalidInput, (std::string option), ());
};

class MockIO : public StreamIO
{
public:
    MockIO(std::ostream& os, std::istream& is, MockMenuActions& actions)
        : StreamIO(os, is)
        , actions_(actions)
    {
    }

    void invalidInput() override
    {
        actions_.invalidInput(currentPrompt());
    }

private:
    MockMenuActions& actions_;
};

TEST(MenuTest, BasicNavigationLetters)
{
    std::istringstream iss("v\ns\ne");
    std::ostringstream oss;

    Menu m("top");
    MockMenuActions acts;
    StreamIO io(oss, iss);

    // Define menu structure and bind lambdas
    m.add("view", Matchers::Key('v'), [&](UserIO& io) {
        acts.optionSelected(io.currentPrompt());
        return Navigation::Continue;
    });
    m.add("select", Matchers::Key('s'), [&](UserIO& io) {
        acts.optionSelected(io.currentPrompt());
        return Navigation::Continue;
    });
    m.add("edit", Matchers::Key('e'), [&](UserIO& io) {
        acts.optionSelected(io.currentPrompt());
        return Navigation::Continue;
    });

    // Set expectation
    InSequence seq;
    EXPECT_CALL(acts, optionSelected("v")).Times(1);
    EXPECT_CALL(acts, optionSelected("s")).Times(1);
    EXPECT_CALL(acts, optionSelected("e")).Times(1);

    // Act
    io.run(m, false);
}

TEST(MenuTest, BasicNavigationNumbers)
{
    std::istringstream iss("9\n10\n11\n12");
    std::ostringstream oss;

    Menu m("top");
    MockMenuActions acts;
    StreamIO io(oss, iss);

    // Define menu structure and bind lambdas
    m.add("range", Matchers::Range(9, 12), [&](UserIO& io) {
        acts.optionSelected(io.currentPrompt());
        return Navigation::Continue;
    });

    // Set expectation
    InSequence seq;
    EXPECT_CALL(acts, optionSelected("9")).Times(1);
    EXPECT_CALL(acts, optionSelected("10")).Times(1);
    EXPECT_CALL(acts, optionSelected("11")).Times(1);
    EXPECT_CALL(acts, optionSelected("12")).Times(1);

    // Act
    io.run(m, false);
}

TEST(MenuTest, SubMenuNavigation)
{
    std::istringstream iss("v\ni\nb\nv\nq");
    std::ostringstream oss;

    Menu m("top");
    MockMenuActions acts;
    StreamIO io(oss, iss);

    // Define menu structure and bind lambdas
    m.add("view", Matchers::Key('v'), [&](UserIO& io) {
        acts.optionSelected(io.currentPrompt());

        Menu sm("sub");
        sm.add("inner", Matchers::Key('i'), [&](UserIO& io) {
            acts.optionSelected(io.currentPrompt());
            return Navigation::Continue;
        });

        return io.run(sm);
    });

    // Set expectation
    InSequence seq;
    EXPECT_CALL(acts, optionSelected("v")).Times(1);

    // Should be triggered as we will be in sub menu
    EXPECT_CALL(acts, optionSelected("i")).Times(1);

    // Now we went back (as b is simulated) to main menu.
    // Verify that `v` can be selected again
    EXPECT_CALL(acts, optionSelected("v")).Times(1);

    // Act
    io.run(m, false);
}

TEST(MenuTest, WrongInput)
{
    std::istringstream iss("oops");
    std::ostringstream oss;

    Menu m("top");
    MockMenuActions acts;
    MockIO io(oss, iss, acts);

    m.add("view", Matchers::Key('v'), [&](UserIO& io) {
        acts.optionSelected(io.currentPrompt());
        return Navigation::Continue;
    });

    // Set expectation
    InSequence seq;
    EXPECT_CALL(acts, invalidInput("oops")).Times(1);
    EXPECT_CALL(acts, optionSelected(_)).Times(0);

    io.run(m, false);
}


} // namespace tools::dups
