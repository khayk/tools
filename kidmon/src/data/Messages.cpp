#include <kidmon/data/Messages.h>
#include <core/utils/Str.h>

#include <nlohmann/json.hpp>

namespace km::msgs {

void buildAuthMsg(std::string_view authToken,
                  std::string_view username,
                  nlohmann::ordered_json& js)
{
    js = {{"name", "auth"},
          {"message",
           {{"username", username},
            {"token", authToken}}}};
}

void buildDataMsg(const Entry& entry, nlohmann::ordered_json& js)
{
    js["name"] = "data";
    auto& msgJs = js["message"];
    msgJs["username"] = entry.username;
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

} // namespace km::msgs
