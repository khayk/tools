#include <gtest/gtest.h>

#include <core/utils/File.h>
#include <core/utils/Crypto.h>

#include <array>
#include <filesystem>

using namespace crypto;
namespace fs = std::filesystem;

namespace {

TEST(UtilsCryptoTests, DataSha256)
{
    std::unordered_map<std::string, std::string> hashes {
        {"e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", ""},
        {"2e1cfa82b035c26cbbbdae632cea070514eb8b773f616aaeaf668e2f0be8f10d", "empty"},
        {"9033b8324ab907bcbb0d2f2bf1c49f57c2d59809364b1940d1e4fac10281841b", "/some/file/path"},
        {"6b86b273ff34fce19d6b804eff5a3f5747ada4eaa22f1d49c01e52ddb7875b4b", "1"},
        {"36a9e7f1c95b82ffb99743e0c5c4ce95d83c9a430aac59f84ef3cbfab6145068", " "}};

    for (auto& pair : hashes)
    {
        EXPECT_EQ(pair.first, sha256(pair.second));
    }
}

TEST(UtilsCryptoTests, DataMd5)
{
    std::unordered_map<std::string, std::string> hashes {
        {"d41d8cd98f00b204e9800998ecf8427e", ""},
        {"a2e4822a98337283e39f7b60acf85ec9", "empty"},
        {"4d436e8c3d135e4bbec1c55c23a27d7f", "/some/file/path"},
        {"c4ca4238a0b923820dcc509a6f75849b", "1"},
        {"7215ee9c7d9dc229d2921a40e899ec5f", " "}};

    for (auto& pair : hashes)
    {
        EXPECT_EQ(pair.first, md5(pair.second));
    }
}

TEST(UtilsCryptoTests, FileSha256)
{
    std::array data {
        std::make_tuple<std::string, std::string>(
            "01234567",
            "924592b9b103f14f833faafb67f480691f01988aa457c0061769f58cd47311bc"),
        std::make_tuple<std::string, std::string>(
            "",
            "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855")};
    const fs::path filename = "dummy.txt";

    for (const auto& dt : data)
    {
        file::write(filename, std::get<0>(dt));

        const auto actual = fileSha256(filename);
        const std::string& expected = std::get<1>(dt);

        EXPECT_EQ(actual, expected);
    }

    fs::remove(filename);
}


TEST(UtilsCryptoTests, CheckEncodeDecode64)
{
    // Holds byte representation of data and its base64 encoding
    std::vector<std::pair<std::string, std::string>> testData;

    testData.emplace_back("", "");
    testData.emplace_back("abcd", "YWJjZA==");
    testData.emplace_back(
        "hdsajfhsufhssfh632746324h2462h32k4324327482h4",
        "aGRzYWpmaHN1Zmhzc2ZoNjMyNzQ2MzI0aDI0NjJoMzJrNDMyNDMyNzQ4Mmg0");
    testData.emplace_back("48029347348274234242432746",
                          "NDgwMjkzNDczNDgyNzQyMzQyNDI0MzI3NDY=");
    testData.emplace_back("Hello world! How're you doing?",
                          "SGVsbG8gd29ybGQhIEhvdydyZSB5b3UgZG9pbmc/");
    testData.emplace_back("<!------------>", "PCEtLS0tLS0tLS0tLS0+");
    testData.emplace_back("1", "MQ==");
    testData.emplace_back("12", "MTI=");
    testData.emplace_back("123", "MTIz");
    testData.emplace_back("1234", "MTIzNA==");
    testData.emplace_back("12345", "MTIzNDU=");

    std::string encoded;
    std::string decoded;

    for (auto& i : testData)
    {
        crypto::encodeBase64(i.first, encoded);
        EXPECT_EQ(encoded, i.second);

        std::string encoded2 = crypto::encodeBase64(i.first);
        EXPECT_EQ(encoded, encoded2);
    }

    for (auto& i : testData)
    {
        crypto::decodeBase64(i.second, decoded);
        EXPECT_EQ(decoded, i.first);

        std::string decoded2 = crypto::decodeBase64(i.second);
        EXPECT_EQ(decoded, decoded2);
    }
}

} // namespace