#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <core/utils/Throw.h>

#include <source_location>
#include <stdexcept>

using testing::HasSubstr;

// Defined at file scope so function_name() does not contain "(anonymous namespace)",
// which would break formatFuncName's parenthesis-based param stripping.
static std::string captureThrowMessage()
{
    try
    {
        core::throwNotImplemented();
    }
    catch (const std::runtime_error& e)
    {
        return e.what();
    }
    return {};
}

namespace {

// ---------------------------------------------------------------------------
// throwNotImplemented
// ---------------------------------------------------------------------------

TEST(ThrowTests, ThrowsRuntimeError)
{
    EXPECT_THROW(core::throwNotImplemented(), std::runtime_error);
}

TEST(ThrowTests, MessageHasNotImplementedPrefix)
{
    try
    {
        core::throwNotImplemented();
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_THAT(std::string(e.what()), HasSubstr("Not implemented:"));
    }
}

TEST(ThrowTests, MessageContainsCallerFileName)
{
    // Capture the expected filename at the call site so the check is resilient
    // to file renames without hardcoding the string.
    const auto loc = std::source_location::current();
    const std::string_view fullPath = loc.file_name();
    const auto sep = fullPath.find_last_of("/\\");
    const std::string file(sep == std::string_view::npos ? fullPath
                                                         : fullPath.substr(sep + 1));
    try
    {
        core::throwNotImplemented();
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_THAT(std::string(e.what()), HasSubstr(file));
    }
}

TEST(ThrowTests, MessageContainsLineNumber)
{
    int line = 0;
    try
    {
        line = static_cast<int>(std::source_location::current().line()) + 1;
        core::throwNotImplemented();
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_THAT(std::string(e.what()), HasSubstr(std::to_string(line)));
    }
}

TEST(ThrowTests, MessageContainsFunctionName)
{
    // captureThrowMessage is a named free function, so formatFuncName can extract
    // its name from source_location without being confused by "(anonymous namespace)".
    EXPECT_THAT(captureThrowMessage(), HasSubstr("captureThrowMessage"));
}

} // namespace
