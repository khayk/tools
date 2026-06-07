#pragma once

#include "Types.h"

namespace km::msgs {

void buildAuthMsg(std::string_view authToken,
                  std::string_view username,
                  nlohmann::ordered_json& js);

void buildDataMsg(const Entry& entry, nlohmann::ordered_json& js);

/**
 * @brief Build a heartbeat message.
 *
 * Sent by the agent in place of a data message while the user is idle: it keeps
 * the connection alive (so the server's peer-drop timer does not fire) without
 * recording activity that would inflate time-spent figures.
 *
 * @param upTimeMs        Agent process uptime, in milliseconds.
 * @param lastActivityMs  Epoch milliseconds of the last detected user input.
 */
void buildHeartbeat(int64_t upTimeMs,
                    int64_t lastActivityMs,
                    nlohmann::ordered_json& js);

void buildResponse(int status,
                   std::string_view error,
                   const nlohmann::json& answer,
                   nlohmann::ordered_json& js);

void buildResponse(int status,
                   const nlohmann::json& answer,
                   nlohmann::ordered_json& js);

bool isAuthMsg(const nlohmann::ordered_json& js);
bool isDataMsg(const nlohmann::ordered_json& js);
bool isHeartbeatMsg(const nlohmann::ordered_json& js);

} // namespace km::msgs
