// Compile Boost.JSON once, using the already pinned Boost distribution.
#include <boost/json/src.hpp>
#include "json.hpp"
namespace nijiexpose::json {
const Value* find(const Value& value, std::string_view name) {
    if (!value.is_object()) return nullptr;
    return value.as_object().if_contains(name);
}
const Value& require(const Value& value, std::string_view name) {
    if (const auto* found = find(value, name)) return *found;
    throw std::runtime_error("Missing JSON member: " + std::string(name));
}
Value parse(std::string_view text) {
    boost::json::parse_options options;
    options.max_depth = std::numeric_limits<std::size_t>::max();
    return boost::json::parse(text, {}, options);
}
std::string serialize(const Value& value) { return boost::json::serialize(value); }
}
