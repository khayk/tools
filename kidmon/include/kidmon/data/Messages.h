#pragma once

#include "Types.h"

namespace msgs {

void buildAuthMsg(std::string_view authToken, nlohmann::ordered_json& js);
void buildDataMsg(const Entry& entry, nlohmann::ordered_json& js);

void buildAnswer(int status, 
                 const std::string_view error, 
                 const nlohmann::json& answer, 
                 nlohmann::ordered_json& js);

void buildAnswer(int status, const nlohmann::json& answer, nlohmann::ordered_json& js);

bool isAuthMsg(const nlohmann::ordered_json& js);
bool isDataMsg(const nlohmann::ordered_json& js);

} // namespace msgs