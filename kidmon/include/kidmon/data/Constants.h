#pragma once

#include <string_view>

namespace km::constants {

constexpr std::string_view PROC_INFO = "proc";
constexpr std::string_view PROC_PATH = "path";
constexpr std::string_view PROC_SHA = "sha256";

constexpr std::string_view WND_INFO = "wnd";
constexpr std::string_view WND_TITLE = "title";
constexpr std::string_view WND_LEFT_TOP = "lt";
constexpr std::string_view WND_DIMENSIONS = "wh";
constexpr std::string_view WND_IMG = "img";

constexpr std::string_view WND_IMG_NAME = "name";
constexpr std::string_view WND_IMG_BYTES = "bytes";
constexpr std::string_view WND_IMG_ENCODED = "encoded";

constexpr std::string_view TIMESTAMP = "ts";
constexpr std::string_view TIMESTAMP_WHEN = "when";
constexpr std::string_view TIMESTAMP_DUR = "dur";

// Environment variable carrying the agent authorization token from the server
// to the spawned agent. Passed via the environment (not argv) so the secret is
// not visible to other users in the process list.
constexpr std::string_view ENV_AUTH_TOKEN = "KIDMON_TOKEN";

} // namespace km::constants
