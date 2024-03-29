#pragma once

#include "MsgHandler.h"

class AuthorizationHandler : public MsgHandler
{
    std::string token_;

public:
    void setToken(std::string_view token);

    bool handle(const nlohmann::json& payload,
                nlohmann::json& answer,
                std::string& error) override;
};
