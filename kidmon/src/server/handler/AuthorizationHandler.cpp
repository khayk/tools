#include <kidmon/server/handler/AuthorizationHandler.h>

#include <nlohmann/json.hpp>

void AuthorizationHandler::setToken(std::string_view token)
{
    token_ = token;
}

bool AuthorizationHandler::handle(const nlohmann::json& req,
                                  nlohmann::json& resp,
                                  std::string& error)
{
    const auto it = req.find("token");

    if (it != req.end())
    {
        std::string token;
        (*it).get_to(token);

        if (token != token_)
        {
            error.assign("Invalid authorization token");
        }

        resp["authorized"] = error.empty();
        return true;
    }

    return false;
}
