#pragma once

#include <nlohmann/json_fwd.hpp>
#include <string>

class MsgHandler
{
protected:
    MsgHandler() = default;
    MsgHandler(const MsgHandler&) = delete;
    MsgHandler(MsgHandler&&) noexcept = delete;
    MsgHandler& operator=(const MsgHandler&) = delete;
    MsgHandler& operator=(MsgHandler&&) noexcept = delete;

public:
    virtual ~MsgHandler() = default;

    virtual bool handle(const nlohmann::json& req,
                        nlohmann::json& resp,
                        std::string& error) = 0;
};