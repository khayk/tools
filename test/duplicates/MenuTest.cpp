#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <duplicates/Menu.h>

using ::testing::_;
using ::testing::InSequence;

class MockMenuActions {
public:
    MOCK_METHOD(void, optionSelected, (std::string option), ());
};

namespace tools::dups {

TEST(MenuTest, BasicNavigationLetters)
{
    std::istringstream iss("v\ns\ne");
    std::ostringstream oss;

    Menu m("Main");
    StreamRenderer renderer(oss, iss);
    MockMenuActions mock;

    // Define menu structure and bind lambdas
    m.add("view", Matchers::Key('v'), [&](Renderer& r) {
        mock.optionSelected(r.currentPrompt());
        return Navigation::Continue;
    });
    m.add("select", Matchers::Key('s'), [&](Renderer& r)  {
        mock.optionSelected(r.currentPrompt());
        return Navigation::Continue;
    });
    m.add("edit", Matchers::Key('e'), [&](Renderer& r) {
        mock.optionSelected(r.currentPrompt());
        return Navigation::Continue;
    });

    // Set expectation
    InSequence seq;
    EXPECT_CALL(mock, optionSelected("v")).Times(1);
    EXPECT_CALL(mock, optionSelected("s")).Times(1);
    EXPECT_CALL(mock, optionSelected("e")).Times(1);

    // Act
    renderer.run(m, false);
}

TEST(MenuTest, BasicNavigationNumbers)
{
    std::istringstream iss("9\n10\n11\n12");
    std::ostringstream oss;

    Menu m("Main");
    StreamRenderer renderer(oss, iss);
    MockMenuActions mock;

    // Define menu structure and bind lambdas
    m.add("range", Matchers::Range(9, 12), [&](Renderer& r) {
        mock.optionSelected(r.currentPrompt());
        return Navigation::Continue;
    });

    // Set expectation
    InSequence seq;
    EXPECT_CALL(mock, optionSelected("9")).Times(1);
    EXPECT_CALL(mock, optionSelected("10")).Times(1);
    EXPECT_CALL(mock, optionSelected("11")).Times(1);
    EXPECT_CALL(mock, optionSelected("12")).Times(1);

    // Act
    renderer.run(m, false);
}

} // namespace tools::dups
