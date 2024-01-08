#include <kidmon/server/handler/DataHandler.h>

#include <nlohmann/json.hpp>

bool DataHandler::handle(const nlohmann::json& req,
                         nlohmann::json& resp,
                         std::string& error)
{
    std::ignore = req;
    std::ignore = resp;
    std::ignore = error;

    resp = R"({"status":"ok"})";

    return false;
}
