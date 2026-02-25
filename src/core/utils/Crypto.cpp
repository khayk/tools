#include <core/utils/Crypto.h>
#include <core/utils/FmtExt.h>

#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/md5.h>

#include <fmt/format.h>

#include <fstream>
#include <cassert>
#include <array>
#include <span>
#include <format>
#include <utility>


namespace crypto {
namespace {
void hexadecimal(std::span<unsigned char> hash, std::string& out)
{
    out.clear();
    out.reserve(hash.size() * 2);

    try
    {
        for (const auto& val : hash)
        {
            // Format each byte as two lowercase hexadecimal digits, zero-padded.
            // Cast std::byte to unsigned char for formatting as an integer value.
            std::format_to(std::back_inserter(out),
                           "{:02x}",
                           static_cast<unsigned char>(val));
        }
    }
    catch (const std::format_error& e)
    {
        throw std::runtime_error(std::string("Hex formatting error: ") + e.what());
    }
}
} // namespace

void sha256(const std::string_view data, std::string& out)
{
    std::array<unsigned char, SHA256_DIGEST_LENGTH> hash {};
    const auto* in = reinterpret_cast<const unsigned char*>(data.data());
    SHA256(in, data.size(), hash.data());
    hexadecimal(std::span(hash), out);
}

std::string sha256(const std::string_view data)
{
    std::string out;
    sha256(data, out);

    return out;
}

void md5(std::string_view data, std::string& out)
{
    uint32_t mdLen = 0;
    std::array<unsigned char, MD5_DIGEST_LENGTH> hash {};
    const EVP_MD* md = EVP_get_digestbyname("MD5");

    if (!md)
    {
        throw std::runtime_error("Failed to initialize MD5 digest");
    }

    auto sslFailed = [](int ret) -> bool {
        return ret != 1;
    };

    std::unique_ptr<EVP_MD_CTX, void (*)(EVP_MD_CTX*)> ctx(EVP_MD_CTX_new(),
                                                           &EVP_MD_CTX_free);

    if (!ctx)
    {
        throw std::runtime_error("Failed to allocate digest context");
    }

    if (sslFailed(EVP_DigestInit_ex(ctx.get(), md, nullptr)))
    {
        throw std::runtime_error("EVP_DigestInit_ex failed");
    }

    if (sslFailed(EVP_DigestUpdate(ctx.get(), data.data(), data.length())))
    {
        throw std::runtime_error("EVP_DigestUpdate failed");
    }

    if (sslFailed(EVP_DigestFinal_ex(ctx.get(), hash.data(), &mdLen)))
    {
        throw std::runtime_error("EVP_DigestFinal_ex failed");
    }

    hexadecimal(std::span(hash), out);
}

std::string md5(std::string_view data)
{
    std::string out;
    md5(data, out);

    return out;
}

std::string fileSha256(const fs::path& file)
{
    std::ifstream in(file, std::ios::in | std::ios::binary);

    if (!in)
    {
        const auto s = fmt::format("Unable to open file: {}", file);
        throw std::system_error(
            std::make_error_code(std::errc::no_such_file_or_directory),
            s);
    }

    constexpr const std::size_t bufferSize {static_cast<unsigned long long>(1UL)
                                            << 12};
    std::array<char, bufferSize> buffer {};
    std::array<unsigned char, EVP_MAX_MD_SIZE> hash {};

    std::unique_ptr<EVP_MD_CTX, void (*)(EVP_MD_CTX* ctx)> ctx(EVP_MD_CTX_new(),
                                                               EVP_MD_CTX_free);

    const EVP_MD* md = EVP_get_digestbyname("sha256");
    EVP_DigestInit_ex(ctx.get(), md, nullptr);

    while (in)
    {
        in.read(buffer.data(), bufferSize);
        if (!EVP_DigestUpdate(ctx.get(),
                              buffer.data(),
                              static_cast<size_t>(in.gcount())))
        {
            throw std::runtime_error("Digest update failed");
        }
    }

    uint32_t mdLen = 0;
    EVP_DigestFinal_ex(ctx.get(), hash.data(), &mdLen);
    in.close();

    std::string out;
    hexadecimal(std::span(hash.data(), mdLen), out);

    return out;
}

void encodeBase64(std::string_view byteSeq, std::string& base64Seq)
{
    const auto len = 4 * ((byteSeq.size() + 2) / 3);
    base64Seq.resize(len);
    const auto res =
        EVP_EncodeBlock(reinterpret_cast<unsigned char*>(base64Seq.data()),
                        reinterpret_cast<const unsigned char*>(byteSeq.data()),
                        static_cast<int>(byteSeq.size()));

    if (std::cmp_not_equal(res, len))
    {
        const auto s = fmt::format("Encode predicted {} but we got {}", len, res);
        throw std::system_error(std::make_error_code(std::errc::result_out_of_range),
                                s);
    }
}

std::string encodeBase64(std::string_view byteSeq)
{
    std::string base64Seq;
    encodeBase64(byteSeq, base64Seq);

    return base64Seq;
}

void decodeBase64(const std::string& base64Seq, std::string& byteSeq)
{
    const auto len = 3 * base64Seq.size() / 4;
    byteSeq.resize(len);
    const auto res =
        EVP_DecodeBlock(reinterpret_cast<unsigned char*>(byteSeq.data()),
                        reinterpret_cast<const unsigned char*>(base64Seq.data()),
                        static_cast<int>(base64Seq.size()));
    if (std::cmp_not_equal(res, len))
    {
        const auto s = fmt::format("Encode predicted {} but we got {}", len, res);
        throw std::system_error(std::make_error_code(std::errc::result_out_of_range),
                                s);
    }

    while (!byteSeq.empty() && byteSeq.back() == 0)
    {
        byteSeq.pop_back();
    }
}

std::string decodeBase64(const std::string& base64Seq)
{
    std::string byteSeq;
    decodeBase64(base64Seq, byteSeq);

    return byteSeq;
}

} // namespace crypto