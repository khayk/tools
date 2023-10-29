#include "Crypto.h"
#include "FmtExt.h"

#include <openssl/evp.h>
#include <openssl/sha.h>

#include <fmt/format.h>

#include <fstream>
#include <array>

namespace crypto {
namespace {
void hexadecimal(unsigned char* const hash, size_t size, std::string& out)
{
    out.resize(2 * size);

    for (size_t i = 0; i < size; ++i)
    {
        sprintf(out.data() + 2 * i, "%02x", hash[i]);
    }
}
}   // namespace 

void sha256(const std::string_view data, std::string& out)
{
    std::array<unsigned char, SHA256_DIGEST_LENGTH> hash {};
    auto in = reinterpret_cast<const unsigned char*>(data.data());
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

    if (!in.good())
    {
        const auto s = fmt::format("Unable to open file: {}", file);
        throw std::system_error(
            std::make_error_code(std::errc::no_such_file_or_directory),
            s);
    }

    constexpr const std::size_t bufferSize {static_cast<unsigned long long>(1UL) << 12};
    char buffer[bufferSize];
    unsigned char hash[EVP_MAX_MD_SIZE] = {0};

    std::unique_ptr<EVP_MD_CTX, void (*)(EVP_MD_CTX* ctx)> ctx(EVP_MD_CTX_new(),
                                                               EVP_MD_CTX_free);

    const EVP_MD* md = EVP_get_digestbyname("sha256");
    EVP_DigestInit_ex(ctx.get(), md, nullptr);

    while (in)
    {
        in.read(buffer, bufferSize);
        if (!EVP_DigestUpdate(ctx.get(), buffer, in.gcount()))
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

} // namespace crypto