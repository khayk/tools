#include <core/utils/Crypto.h>
#include <core/utils/FmtExt.h>

#include <openssl/evp.h>
#include <openssl/sha.h>

#include <fmt/format.h>

#include <fstream>
#include <cassert>
#include <array>

namespace crypto {
namespace {
void hexadecimal(unsigned char* const hash, size_t size, std::string& out)
{
    out.resize(2 * size);

    for (size_t i = 0; i < size; ++i)
    {
        int written = 0;
#ifdef _WIN32
        written = sprintf_s(out.data() + 2 * i, 3, "%02x", hash[i]);
#else
        written = snprintf(out.data() + 2 * i, 3, "%02x", hash[i]);
#endif
        assert(written >= 0);
        if (written < 0)
        {
            throw std::runtime_error("Failed to write in hexadecimal format");
        }
    }
}
} // namespace

void sha256(const std::string_view data, std::string& out)
{
    std::array<unsigned char, SHA256_DIGEST_LENGTH> hash {};
    const auto* in = reinterpret_cast<const unsigned char*>(data.data());
    SHA256(in, data.size(), hash.data());
    hexadecimal(hash.data(), hash.size(), out);
}

std::string sha256(const std::string_view data)
{
    std::string out;
    sha256(data, out);

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
    char buffer[bufferSize];
    unsigned char hash[EVP_MAX_MD_SIZE] = {0};

    std::unique_ptr<EVP_MD_CTX, void (*)(EVP_MD_CTX* ctx)> ctx(EVP_MD_CTX_new(),
                                                               EVP_MD_CTX_free);

    const EVP_MD* md = EVP_get_digestbyname("sha256");
    EVP_DigestInit_ex(ctx.get(), md, nullptr);

    while (in)
    {
        in.read(buffer, bufferSize);
        if (!EVP_DigestUpdate(ctx.get(), buffer, static_cast<size_t>(in.gcount())))
        {
            throw std::runtime_error("Digest update failed");
        }
    }

    uint32_t mdLen = 0;
    EVP_DigestFinal_ex(ctx.get(), hash, &mdLen);
    in.close();

    std::string out;
    hexadecimal(hash, mdLen, out);

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

    if (res != static_cast<int>(len))
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
    if (res != static_cast<int>(len))
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