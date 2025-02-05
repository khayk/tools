#include <kidmon/data/Messages.h>
#include <core/utils/Str.h>
#include <core/utils/Sys.h>

#include <nlohmann/json.hpp>

namespace msgs {

void buildAuthMsg(std::string_view authToken, nlohmann::ordered_json& js)
{
    js = {{"name", "auth"},
          {"message",
           {{"username", str::ws2s(sys::activeUserName())}, {"token", authToken}}}};
}

void buildDataMsg(const Entry& entry, nlohmann::ordered_json& js)
{
    js["name"] = "data";
    auto& msgJs = js["message"];
    msgJs["username"] = str::ws2s(sys::activeUserName());
    toJson(entry, msgJs["entry"]);
}

void buildResponse(int status,
                   const std::string_view error,
                   const nlohmann::json& answer,
                   nlohmann::ordered_json& js)
{
    js["status"] = status;
    if (!error.empty())
    {
        js["error"] = std::string(error);
    }
    if (!answer.is_null() && !answer.empty())
    {
        js["answer"] = answer;
    }
}

void buildResponse(int status,
                   const nlohmann::json& answer,
                   nlohmann::ordered_json& js)
{
    buildResponse(status, "", answer, js);
}

bool isAuthMsg(const nlohmann::ordered_json& js)
{
    const auto nit = js.find("name");
    return !(nit == js.end() || (*nit).get<std::string>() != "auth");
}

bool isDataMsg(const nlohmann::ordered_json& js)
{
    const auto nit = js.find("name");
    return !(nit == js.end() || (*nit).get<std::string>() != "data");
}

} // namespace msgs