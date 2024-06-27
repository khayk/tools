#include <kidmon/server/handler/DataHandler.h>
#include <kidmon/common/Utils.h>
#include <kidmon/data/Types.h>
#include <core/utils/Crypto.h>
#include <core/utils/Str.h>

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <spdlog/spdlog.h>


DataHandler::DataHandler(IRepository& repo)
    : repo_(repo)
{
}


bool DataHandler::handle(const nlohmann::json& payload,
                         nlohmann::json& answer,
                         std::string& error)
{
    try
    {
        const auto& msg = payload["message"];
        std::ignore = answer;

        Entry entry;
        fromJson(msg["entry"], entry);
        entry.username = msg["username"];

        if (entry.username.empty() || entry.username != str::ws2s(sys::activeUserName()))
        {
            error = "Username mismatch";
            return false;
        }

        const std::string& imageName = entry.windowInfo.image.name;
        const std::string& imageBytes = entry.windowInfo.image.bytes;
        const bool hasSnapshot = !imageName.empty();

        if (hasSnapshot)
        {
            crypto::decodeBase64(imageBytes, buffer_);
            entry.windowInfo.image.encoded = false;
            entry.windowInfo.image.bytes = buffer_;
        }

        repo_.add(entry);

        return true;
    }
    catch (const std::exception& ex)
    {
        error = fmt::format("Unable to handle incoming payload: {}", ex.what());
    }

    return false;
}
