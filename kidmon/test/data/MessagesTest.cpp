#include <gtest/gtest.h>

#include <kidmon/data/Messages.h>

#include <nlohmann/json.hpp>

using namespace km;

namespace {

TEST(MessagesTest, ResponseSuccessHasZeroStatusAndNoError)
{
    nlohmann::json answer;
    answer["authorized"] = true;

    nlohmann::ordered_json js;
    msgs::buildResponse(0, "", answer, js);

    EXPECT_EQ(js["status"].get<int>(), 0);
    EXPECT_FALSE(js.contains("error")); // empty error is omitted
    EXPECT_TRUE(js["answer"]["authorized"].get<bool>());
}

TEST(MessagesTest, ResponseFailureCarriesStatusAndError)
{
    nlohmann::ordered_json js;
    msgs::buildResponse(1, "Invalid authorization token", nlohmann::json {}, js);

    EXPECT_NE(js["status"].get<int>(), 0);
    EXPECT_EQ(js["error"].get<std::string>(), "Invalid authorization token");
    EXPECT_FALSE(js.contains("answer")); // null/empty answer is omitted
}

TEST(MessagesTest, ResponseFailureKeepsAnswerWhenPresent)
{
    nlohmann::json answer;
    answer["authorized"] = false;

    nlohmann::ordered_json js;
    msgs::buildResponse(1, "An authorized agent already exists", answer, js);

    EXPECT_NE(js["status"].get<int>(), 0);
    EXPECT_EQ(js["error"].get<std::string>(), "An authorized agent already exists");
    EXPECT_FALSE(js["answer"]["authorized"].get<bool>());
}

} // namespace
