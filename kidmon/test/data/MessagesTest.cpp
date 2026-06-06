#include <gtest/gtest.h>

#include <kidmon/data/Messages.h>

#include <nlohmann/json.hpp>

using namespace km;

namespace {

TEST(MessagesTest, HeartbeatRoundTrips)
{
    nlohmann::ordered_json js;
    msgs::buildHeartbeat(12'345, 12'300, js);

    EXPECT_TRUE(msgs::isHeartbeatMsg(js));
    EXPECT_FALSE(msgs::isDataMsg(js));
    EXPECT_FALSE(msgs::isAuthMsg(js));

    const auto& msg = js["message"];
    EXPECT_EQ(msg["up_time_ms"].get<int64_t>(), 12'345);
    EXPECT_EQ(msg["last_activity_time_ms"].get<int64_t>(), 12'300);
}

TEST(MessagesTest, MessageTypePredicatesAreDistinct)
{
    nlohmann::ordered_json data;
    msgs::buildDataMsg(Entry {}, data);
    EXPECT_TRUE(msgs::isDataMsg(data));
    EXPECT_FALSE(msgs::isHeartbeatMsg(data));

    nlohmann::ordered_json auth;
    msgs::buildAuthMsg("token", "user", auth);
    EXPECT_TRUE(msgs::isAuthMsg(auth));
    EXPECT_FALSE(msgs::isHeartbeatMsg(auth));
}

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
