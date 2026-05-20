#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <core/utils/Throw.h>

#include <source_location>
#include <stdexcept>

using testing::HasSubstr;
using testing::Not;

// All helpers live at file scope (not inside anonymous namespace) so that
// source_location::function_name() does not contain "(anonymous namespace)",
// which would cause formatFuncName to truncate the name to an empty string.

static std::string captureThrowMessage()
{
    std::string result;
    try
    {
        core::throwNotImplemented();
    }
    catch (const std::runtime_error& e)
    {
        result = e.what();
    }
    return result;
}

// Member of a class template — function_name() contains '<T>' on all major
// compilers, which exercises the ++depth / --depth template-stripping loop.
template <typename T>
struct TypedCapture
{
    static std::string capture()
    {
        std::string result;
        try
        {
            core::throwNotImplemented();
        }
        catch (const std::runtime_error& e)
        {
            result = e.what();
        }
        return result;
    }
};

// Named lowercase namespace — formatFuncName should strip it and return only
// the function or class name.
namespace throwtest {

std::string captureFromNs()
{
    std::string result;
    try
    {
        core::throwNotImplemented();
    }
    catch (const std::runtime_error& e)
    {
        result = e.what();
    }
    return result;
}

// Class nested inside a namespace — function_name() yields
// "throwtest::NestedHelper::capture", so qualifier = "throwtest::NestedHelper"
// and qualSep != npos, exercising the qualifier.substr(qualSep + 2) branch.
struct NestedHelper
{
    static std::string capture()
    {
        std::string result;
        try
        {
            core::throwNotImplemented();
        }
        catch (const std::runtime_error& e)
        {
            result = e.what();
        }
        return result;
    }
};

} // namespace throwtest

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
    // its name without being confused by "(anonymous namespace)".
    EXPECT_THAT(captureThrowMessage(), HasSubstr("captureThrowMessage"));
}

// ---------------------------------------------------------------------------
// formatFuncName branch coverage
// ---------------------------------------------------------------------------

TEST(ThrowTests, TemplateParamsAreStrippedFromFunctionName)
{
    // TypedCapture<int>::capture has '<int>' in function_name(), which drives the
    // ++depth / --depth path inside formatFuncName's template-stripping loop.
    EXPECT_THAT(TypedCapture<int>::capture(), HasSubstr("TypedCapture"));
}

TEST(ThrowTests, LowercaseNamespaceQualifierIsStripped)
{
    // "throwtest" starts with a lowercase letter, so formatFuncName treats it as
    // a namespace and returns only the bare function name.
    const auto msg = throwtest::captureFromNs();
    EXPECT_THAT(msg, HasSubstr("captureFromNs"));
    EXPECT_THAT(msg, Not(HasSubstr("throwtest::")));
}

TEST(ThrowTests, DeepQualifierExtractsImmediateClassComponent)
{
    // function_name() = "...throwtest::NestedHelper::capture(...)"
    // qualifier = "throwtest::NestedHelper", qualSep != npos
    // → lastQual = "NestedHelper" (uppercase) → "NestedHelper::capture"
    EXPECT_THAT(throwtest::NestedHelper::capture(), HasSubstr("NestedHelper"));
}

} // namespace
