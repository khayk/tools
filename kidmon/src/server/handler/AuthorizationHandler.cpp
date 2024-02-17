#include <kidmon/server/handler/AuthorizationHandler.h>
#include <kidmon/data/Messages.h>

#include <nlohmann/json.hpp>

void AuthorizationHandler::setToken(std::string_view token)
{
    token_ = token;
}

bool AuthorizationHandler::handle(const nlohmann::json& payload,
                                  nlohmann::json& answer,
                                  std::string& error)
{
    if (!msgs::isAuthMsg(payload))
    {
        return false;
    }

    const auto im = payload.find("message");
    if (im == payload.end())
    {
        return false;
    }

    const auto it = (*im).find("token");
    if (it == (*im).end())
    {
        return false;
    }

    std::string token;
    (*it).get_to(token);

    if (token != token_)
    {
        error.assign("Invalid authorization token");
    }

    answer["authorized"] = error.empty();
    return true;
}
