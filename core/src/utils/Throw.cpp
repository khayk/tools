#include <core/utils/Throw.h>

#include <cctype>
#include <format>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

std::string_view fileNameOnly(std::string_view path)
{
    const auto pos = path.find_last_of("/\\");
    return (pos == std::string_view::npos) ? path : path.substr(pos + 1);
}

std::string formatFuncName(std::string_view full)
{
    // Strip template parameters <...> (handles nesting)
    std::string s;
    s.reserve(full.size());
    int depth = 0;
    for (char c : full)
    {
        if (c == '<')        { ++depth; }
        else if (c == '>')   { --depth; }
        else if (depth == 0) { s += c; }
    }

    // Strip function parameters: everything from the first '('
    if (const auto p = s.find('('); p != std::string::npos)
    {
        s.resize(p);
    }

    // Strip return type: everything up to and including the last space
    if (const auto p = s.rfind(' '); p != std::string::npos)
    {
        s.erase(0, p + 1);
    }

    // s is now "ns::Class::func", "Class::func", or "func"
    const auto sep = s.rfind("::");
    if (sep == std::string::npos)
    {
        return s;
    }

    std::string funcName        = s.substr(sep + 2);
    const std::string qualifier = s.substr(0, sep);

    // Take the immediate qualifier (last :: component before the function)
    const auto qualSep  = qualifier.rfind("::");
    const auto lastQual = (qualSep == std::string::npos)
                              ? qualifier
                              : qualifier.substr(qualSep + 2);

    // Uppercase first char signals a class name, lowercase signals a namespace
    if (!lastQual.empty() && std::isupper(static_cast<unsigned char>(lastQual[0])))
    {
        return lastQual + "::" + funcName;
    }

    return funcName;
}

} // namespace

namespace core {

void throwNotImplemented(std::source_location loc)
{
    throw std::runtime_error(
        std::format("Not implemented: {} ({}:{})",
                    formatFuncName(loc.function_name()),
                    fileNameOnly(loc.file_name()),
                    loc.line()));
}

} // namespace core
