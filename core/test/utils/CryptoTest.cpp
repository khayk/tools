#include <gtest/gtest.h>

#include "utils/File.h"
#include "utils/Crypto.h"

#include <array>
#include <filesystem>

using namespace crypto;
namespace fs = std::filesystem;

namespace {

TEST(UtilsCryptoTests, DataSha256)
{
    EXPECT_EQ(sha256(""),
              "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    EXPECT_EQ(sha256("1"),
              "6b86b273ff34fce19d6b804eff5a3f5747ada4eaa22f1d49c01e52ddb7875b4b");
    EXPECT_EQ(sha256("ab"),
              "fb8e20fc2e4c3f248c60c39bd652f3c1347298bb977b8b4d5903b85055620603");
    EXPECT_EQ(sha256("_045u7"),
              "c9627deaca269fd9b5de61a1093c9f7e7a429f6a2233adf2b3a4fa79129cca4d");
    EXPECT_EQ(sha256("password"),
              "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8");
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

} // namespace

/*
TEST(UtilsCryptoTests, CheckEncodeDecode64)
{
    // Holds byte representation of data and its base64 encoding
    std::vector<std::pair<std::string, std::string>> testData;

    testData.emplace_back("", "");
    testData.emplace_back("abcd", "YWJjZA==");
    testData.emplace_back("hdsajfhsufhssfh632746324h2462h32k4324327482h4",
                          "aGRzYWpmaHN1Zmhzc2ZoNjMyNzQ2MzI0aDI0NjJoMzJrNDMyNDMyNzQ4Mmg0");
    testData.emplace_back("1", "MQ==");
    testData.emplace_back("48029347348274234242432746",
                          "NDgwMjkzNDczNDgyNzQyMzQyNDI0MzI3NDY=");
    testData.emplace_back("Hello world! How're you doing?",
                          "SGVsbG8gd29ybGQhIEhvdydyZSB5b3UgZG9pbmc/");

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
*/