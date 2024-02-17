#include <kidmon/data/Messages.h>
#include <core/utils/Str.h>
#include <kidmon/common/Utils.h>

#include <nlohmann/json.hpp>

namespace msgs {

void buildAuthMsg(std::string_view authToken, nlohmann::ordered_json& js)
{
    js = {
        {"name", "auth"},
        {"message", {{"username", str::ws2s(sys::activeUserName())},
                     {"token", authToken}}}
    };
}

void buildDataMsg(const Entry& entry, nlohmann::ordered_json& js)
{
    js["name"] = "data";
    auto& msgJs = js["message"];
    msgJs["username"] = str::ws2s(sys::activeUserName());
    toJson(entry, msgJs["entry"]);
}

void buildAnswer(int status,
    const std::string_view error,
    const nlohmann::json& answer,
    nlohmann::ordered_json& js)
{
    js["status"] = status;
    js["error"] = std::string(error);
    js["answer"] = answer;
}

void buildAnswer(int status, const nlohmann::json& answer, nlohmann::ordered_json& js)
{
    buildAnswer(status, "", answer, js);
}

bool isAuthMsg(const nlohmann::ordered_json& js)
{
    const auto nit = js.find("name");
    if (nit == js.end() || (*nit).get<std::string>() != "auth")
    {
        return false;
    }

    return true;
}

bool isDataMsg(const nlohmann::ordered_json& js)
{
    const auto nit = js.find("name");
    if (nit == js.end() || (*nit).get<std::string>() != "data")
    {
        return false;
    }

    return true;
}

}  // namespace msgs