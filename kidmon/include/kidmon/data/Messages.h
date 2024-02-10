#pragma once

#include "Types.h"

namespace msgs {

void buildAuthMsg(std::string_view authToken, nlohmann::ordered_json& js);
void buildDataMsg(const Entry& entry, nlohmann::ordered_json& js);

} // namespace msgs