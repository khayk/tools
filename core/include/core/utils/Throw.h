#pragma once

#include <source_location>

namespace core {

[[noreturn]] void throwNotImplemented(
    std::source_location loc = std::source_location::current());

} // namespace core
