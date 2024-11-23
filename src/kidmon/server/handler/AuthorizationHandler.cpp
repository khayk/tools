#include <kidmon/server/handler/AuthorizationHandler.h>
#include <kidmon/data/Messages.h>

#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace {

template <typename T>
bool get(const nlohmann::json& js, std::string_view key, T& dest)
{
    if (const auto it = js.find(key); it != js.end())
    {
        (*it).get_to(dest);
        return true;
    }

    return false;
}

} // namespace

void AuthorizationHandler::setToken(std::string_view token)
{
    token_ = token;
}

const std::string& AuthorizationHandler::username() const noexcept
{
    return username_;
}

bool AuthorizationHandler::handle(const nlohmann::json& payload,
                                  nlohmann::json& answer,
                                  std::string& error)
{
    if (!msgs::isAuthMsg(payload))
    {
        return false;
    }

    nlohmann::json jsMsg;
    if (!get(payload, "message", jsMsg))
    {
        return false;
    }

    std::string token;
    if (!get(jsMsg, "token", token))
    {
        return false;
    }

    if (token != token_)
    {
        error.assign("Invalid authorization token");
    }

    get(jsMsg, "username", username_);
    answer["authorized"] = error.empty();

    return true;
}
