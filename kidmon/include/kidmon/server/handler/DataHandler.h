#pragma once

#include "MsgHandler.h"

class DataHandler : public MsgHandler
{
public:
    bool handle(const nlohmann::json& req,
                nlohmann::json& resp,
                std::string& error) override;
};
