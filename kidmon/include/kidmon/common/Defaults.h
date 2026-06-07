#pragma once

#include <chrono>
#include <cstdint>

// Single source of truth for the agent/server defaults that would otherwise be
// duplicated across KidmonServer::Config and KidmonAgent::Config (and the docs).
namespace km::defaults {

// TCP port the server listens on and the agent connects to. The two must agree.
// Keep the README's "51097" in sync with this value.
inline constexpr uint16_t SERVER_PORT = 51'097;

// How often the agent samples activity (and the server's matching cadence).
inline constexpr std::chrono::milliseconds ACTIVITY_CHECK_INTERVAL {2'000};

// Extra slack added to ACTIVITY_CHECK_INTERVAL to form the server's peer-drop
// timeout, so a single missed sample does not sever the connection.
inline constexpr std::chrono::milliseconds PEER_DROP_GRACE {2'000};

// Minimum gap between agent screenshots.
inline constexpr std::chrono::milliseconds SNAPSHOT_INTERVAL {10'000};

// No keyboard/mouse input (with the screen asleep) for this long marks the user
// away, so idle time is not counted as time-on-task.
inline constexpr std::chrono::milliseconds IDLE_THRESHOLD {60'000};

} // namespace km::defaults
