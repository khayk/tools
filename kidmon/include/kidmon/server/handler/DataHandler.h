#pragma once

#include "MsgHandler.h"

class DataHandler : public MsgHandler
{
public:
    bool handle(const nlohmann::json& payload,
                nlohmann::json& answer,
                std::string& error) override;
};
