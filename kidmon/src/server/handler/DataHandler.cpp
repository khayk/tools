#include <kidmon/server/handler/DataHandler.h>

#include <nlohmann/json.hpp>

bool DataHandler::handle(const nlohmann::json& payload,
                         nlohmann::json& answer,
                         std::string& error)
{
    std::ignore = payload;
    std::ignore = answer;
    std::ignore = error;

    answer = R"({})";

    return false;
}
