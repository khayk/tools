#pragma once

#include "MsgHandler.h"

namespace km {

class AuthorizationHandler : public MsgHandler
{
    std::string token_;
    std::string username_; //< the authenticated username

public:
    void setToken(std::string_view token);

    [[nodiscard]] const std::string& username() const noexcept;

    bool handle(const nlohmann::json& payload,
                nlohmann::json& answer,
                std::string& error) override;
};

} // namespace km
