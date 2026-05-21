#include <core/utils/SingleInstanceChecker.h>
#include <spdlog/spdlog.h>

namespace core {

SingleInstanceChecker::SingleInstanceChecker(std::wstring_view name)
    : appName_(name)
{
    spdlog::error("Single instance checker is not implemented");
}

SingleInstanceChecker::~SingleInstanceChecker() {}

} // namespace core
