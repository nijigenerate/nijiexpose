#pragma once
#include <boost/json.hpp>
#include <array>
#include <charconv>
#include <cmath>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace nijiexpose::json {
using Value = boost::json::value;
using Object = boost::json::object;
using Array = boost::json::array;
// A missing member is a null pointer; an explicit JSON null is a Value.
const Value* find(const Value& value, std::string_view name);
const Value& require(const Value& value, std::string_view name);
Value parse(std::string_view text);
std::string serialize(const Value& value);

template<class T> struct ArrayTraits { static constexpr bool enabled = false; };
template<class T, std::size_t N> struct ArrayTraits<std::array<T, N>> {
    static constexpr bool enabled = true;
    using Element = T;
};
template<class T> T initialValue() {
    if constexpr (std::is_floating_point_v<T>) return std::numeric_limits<T>::quiet_NaN();
    else if constexpr (ArrayTraits<T>::enabled) {
        T value;
        for (auto& item : value) item = initialValue<typename ArrayTraits<T>::Element>();
        return value;
    } else return T{};
}

template<class T> bool deserialize(const Value& value, T& out);
template<class T> Value valueFrom(const T& value);

template<class T> bool deserialize(const Value& value, std::optional<T>& out) {
    if (value.is_null()) { out.reset(); return true; }
    if constexpr (std::is_same_v<T, std::string>) {
        // D's deserializeValue duplicates the slice; an empty duplicate has
        // a null data pointer. The stored JSON string itself stays empty.
        if (value.is_string() && value.as_string().empty()) { out.reset(); return true; }
    }
    T result = out ? *out : initialValue<T>();
    if (!deserialize(value, result)) return false;
    out = std::move(result);
    return true;
}
template<class T> bool deserialize(const Value& value, std::vector<T>& out) {
    if (value.is_null()) { out.clear(); return true; }
    if (!value.is_array()) return false;
    out.assign(value.as_array().size(), initialValue<T>());
    for (std::size_t i = 0; i < out.size(); ++i)
        if (!deserialize(value.as_array()[i], out[i])) return false;
    return true;
}
template<class T> bool deserialize(const Value& value, T& out) {
    if constexpr (requires { out.deserializeFromJson(value); }) {
        out.deserializeFromJson(value);
        return true;
    } else if constexpr (std::is_same_v<T, Value>) { out = value; return true; }
    else if constexpr (std::is_same_v<T, bool>) {
        if (!value.is_bool()) return false;
        out = value.as_bool(); return true;
    } else if constexpr (std::is_same_v<T, std::string>) {
        if (value.is_null()) { out.clear(); return true; }
        if (!value.is_string()) return false;
        out.assign(value.as_string().data(), value.as_string().size()); return true;
    } else if constexpr (ArrayTraits<T>::enabled) {
        if (value.is_null()) return true;
        if (!value.is_array()) return false;
        const auto& array = value.as_array();
        for (std::size_t i = 0; i < out.size() && i < array.size(); ++i)
            if (!deserialize(array[i], out[i])) return false;
        return true;
    } else if constexpr (std::is_arithmetic_v<T>) {
        if constexpr (std::is_floating_point_v<T>) {
            if (value.is_null()) { out = std::numeric_limits<T>::quiet_NaN(); return true; }
            if (value.is_string()) {
                const std::string_view text(value.as_string().data(), value.as_string().size());
                if (text == "nan" || text == "NaN" || text == "NAN" ||
                    text == "+nan" || text == "+NaN" || text == "+NAN" ||
                    text == "-nan" || text == "-NaN" || text == "-NAN") {
                    out = std::numeric_limits<T>::quiet_NaN(); return true;
                }
                if (text == "inf" || text == "Inf" || text == "INF" ||
                    text == "+inf" || text == "+Inf" || text == "+INF") {
                    out = std::numeric_limits<T>::infinity(); return true;
                }
                if (text == "-inf" || text == "-Inf" || text == "-INF") {
                    out = -std::numeric_limits<T>::infinity(); return true;
                }
                T parsed;
                const auto result = std::from_chars(text.data(), text.data() + text.size(), parsed);
                if (result.ec != std::errc{} || result.ptr != text.data() + text.size())
                    throw std::runtime_error("Invalid numeric setting: " + std::string(text));
                out = parsed; return true;
            }
        }
        if (!value.is_number()) return false;
        if constexpr (std::is_floating_point_v<T>) {
            if (value.is_double()) out = static_cast<T>(value.as_double());
            else if (value.is_int64()) out = static_cast<T>(value.as_int64());
            else out = static_cast<T>(value.as_uint64());
        } else {
            // Boost.JSON performs checked conversion; never silently clamp.
            out = boost::json::value_to<T>(value);
        }
        return true;
    } else {
        static_assert(sizeof(T) == 0, "Provide deserializeFromJson for this application type");
    }
}
inline Value valueFrom(std::nullptr_t) { return nullptr; }
inline Value valueFrom(const char* value) { return value ? Value(value) : Value(nullptr); }
template<class T> Value valueFrom(const std::optional<T>& value) { return value ? valueFrom(*value) : Value(nullptr); }
template<class T> Value valueFrom(const std::vector<T>& value) {
    Array array;
    for (const auto& item : value) array.push_back(valueFrom(item));
    return array;
}
template<class T> Value valueFrom(const T& value) {
    if constexpr (requires { value.serializeToJson(); }) return value.serializeToJson();
    else if constexpr (ArrayTraits<T>::enabled) {
        Array array;
        for (const auto& item : value) array.push_back(valueFrom(item));
        return array;
    } else if constexpr (std::is_floating_point_v<T>) {
        if (std::isnan(value)) return std::signbit(value) ? Value("-nan") : Value("nan");
        if (std::isinf(value)) return value > 0 ? Value("inf") : Value("-inf");
        return static_cast<double>(value);
    } else return boost::json::value_from(value);
}
}
