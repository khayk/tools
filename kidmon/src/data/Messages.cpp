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

}  // namespace msgs