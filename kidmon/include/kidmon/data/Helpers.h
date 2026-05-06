#include <nlohmann/json.hpp>
#include <format>

namespace jsu {

template <typename Value>
void get(const nlohmann::json& js,
         std::string_view key,
         Value& v,
         bool required = true)
{
    if (const auto it = js.find(key); it != js.end())
    {
        (*it).get_to(v);
        return;
    }

    if (required)
    {
        throw std::runtime_error(std::format("Missing required key: {}", key));
    }
}

} // namespace jsu
