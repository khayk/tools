#pragma once

#include <string_view>
#include <filesystem>

namespace fs = std::filesystem;

namespace file {

void write(const fs::path& file, std::string_view data);

void write(const fs::path& file, const char* const data, size_t size);

/**
 * @brief Read the content of the file into 'data' string
 *
 * @param file Path to the file.
 * @param data An object to be filled with the file content.
 *
 * @return true if the operation completed successfully, otherwise false
 */
bool read(const fs::path& file, std::string& data, std::error_code& ec);

std::string path2s(const fs::path& path);

} // namespace file