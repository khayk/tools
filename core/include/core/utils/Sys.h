#pragma once

#include <filesystem>

namespace fs = std::filesystem;

namespace sys {

std::wstring activeUserName();

bool isUserInteractive() noexcept;

std::string errorDescription(uint64_t code);

std::string constructErrorMsg(std::string_view message, uint64_t errorCode);

std::string constructLastErrorMsg(std::string_view message);

void logError(std::string_view message, uint64_t errorCode);

void logLastError(std::string_view message);

/**
 * @brief Returns current process id
 */
uint32_t currentProcessId() noexcept;

/**
 * @brief Full path of the current process
 *
 * @return Current process path with executable name.
 */
fs::path currentProcessPath();

/**
 * @brief Returns number of bytes occupied by the process with given pid
 *
 * @param pid  Process ID
 *
 * @return Allocated memory in bytes
 */
size_t processMemoryUsage(uint32_t pid);

/**
 * @brief Returns number of bytes occupied by the current process
 *
 * @return Allocated memory in bytes
 */
size_t currentProcessMemoryUsage();

} // namespace sys