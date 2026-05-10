#pragma once

#include <nlohmann/json_fwd.hpp>
#include <string>

namespace km {

class MsgHandler
{
protected:
    MsgHandler() = default;

public:
    MsgHandler(const MsgHandler&) = delete;
    MsgHandler(MsgHandler&&) noexcept = delete;
    MsgHandler& operator=(const MsgHandler&) = delete;
    MsgHandler& operator=(MsgHandler&&) noexcept = delete;
    virtual ~MsgHandler() = default;

    virtual bool handle(const nlohmann::json& payload,
                        nlohmann::json& answer,
                        std::string& error) = 0;
};

} // namespace km
