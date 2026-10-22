#pragma once
#include <array>
#include <cstdio>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace nijiexpose {
void insInitLogging();
void insLogFormatted(std::string_view level, std::string_view message) noexcept;

namespace logging_detail {
inline std::string text(std::string_view value) { return std::string(value); }
inline std::string text(const std::string& value) { return value; }
inline std::string text(const char* value) { return value ? value : ""; }
inline std::string text(bool value) { return value ? "true" : "false"; }
std::string text(const std::vector<std::string>& values);
template<typename T> std::string text(T value) {
    std::ostringstream stream;
    stream << value;
    return stream.str();
}

// All original insLog call sites use %s (including the startup string array).
// Mismatched or unsupported format strings throw inside insLog's catch boundary.
template<typename... Args>
void insLog(std::string_view level, std::string_view fmt, const Args&... args) noexcept {
    try {
        const std::array<std::string, sizeof...(Args)> arguments{text(args)...};
        std::size_t current = 0;
        std::string message;
        for (std::size_t i = 0; i < fmt.size(); ++i) {
            if (fmt[i] != '%') { message += fmt[i]; continue; }
            if (++i == fmt.size()) throw std::runtime_error("Incomplete format specifier");
            if (fmt[i] == '%') message += '%';
            else if (fmt[i] == 's' && current < arguments.size()) message += arguments[current++];
            else throw std::runtime_error("Invalid log format");
        }
        if (current != arguments.size()) throw std::runtime_error("Unused log argument");
        insLogFormatted(level, message);
    } catch (const std::exception&) {}
}
}

template<typename... Args> void insLogDebug(std::string_view fmt, const Args&... args) { logging_detail::insLog("DEBUG", fmt, args...); }
template<typename... Args> void insLogInfo(std::string_view fmt, const Args&... args) { logging_detail::insLog("INFO", fmt, args...); }
template<typename... Args> void insLogWarn(std::string_view fmt, const Args&... args) { logging_detail::insLog("WARN", fmt, args...); }
template<typename... Args> void insLogErr(std::string_view fmt, const Args&... args) { logging_detail::insLog("ERR ", fmt, args...); }
}
