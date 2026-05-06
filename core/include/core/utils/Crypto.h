#pragma once

#include <string>
#include <filesystem>

namespace fs = std::filesystem;

namespace crypto {

/**
 * @brief Calculate SHA256 on the given data.
 *
 * @param data The data to calculate sha256 on it.
 *
 * @return SHA256 of the data
 */
void sha256(std::string_view data, std::string& out);


/**
 * @brief Convenience function, see sha256 with 2 arguments
 */
std::string sha256(std::string_view data);


/**
 * @brief Calculate MD5 on the given data.
 *
 * @param data The data to calculate MD5 on it.
 *
 * @return MD5 of the data
 */
void md5(std::string_view data, std::string& out);


/**
 * @brief Convenience function, see MD5 with 2 arguments
 */
std::string md5(std::string_view data);


/**
 * @brief Calculate SHA256 of the file.
 *
 * @param filePath The path to the file.
 *
 * @return SHA256 of the file.
 */
std::string fileSha256(const fs::path& file);


/**
 * @brief Convert byte sequence into a base64 sequence
 *
 * @param byteSeq    byte sequence to be encoded
 * @param base64Seq  output base64 sequence, output is automatically resized
 *                    to the needed length
 */
void encodeBase64(std::string_view byteSeq, std::string& base64Seq);


/**
 * @brief Convenience function, see encodeBase64 with 2 arguments
 */
std::string encodeBase64(std::string_view byteSeq);


/**
 * @brief Convert base64 sequence back to byte sequence
 *
 * @param base64Seq  base64 sequence to be decoded
 * @param byteSeq    output byte sequence, output is automatically resized
 *                    to the needed length
 */
void decodeBase64(const std::string& base64Seq, std::string& byteSeq);


/**
 * @brief Convenience function, see decodeBase64 with 2 arguments
 */
std::string decodeBase64(const std::string& base64Seq);

} // namespace crypto