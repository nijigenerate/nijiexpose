// Direct Windows translation of source/nijiexpose/log.d.
// Copyright (c) 2022 Inochi2D Project, 2024 nijigenerate Project. BSD-2-Clause.
#include "nijiexpose/log.hpp"
#include "dcompat/string.hpp"
#include <filesystem>
#include <io.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace nijiexpose {
void insInitLogging() {
    std::wstring destination = L"NUL";
    try {
        const DWORD length = GetEnvironmentVariableW(L"LOCALAPPDATA", nullptr, 0);
        if (length) {
            std::wstring root(length, L'\0');
            GetEnvironmentVariableW(L"LOCALAPPDATA", root.data(), length);
            root.resize(length - 1);
            const auto directory = std::filesystem::path(root) / L"nijiexpose" / L"logs";
            std::filesystem::create_directories(directory);
            const auto path = directory / L"nijiexpose.log";
            if (std::filesystem::exists(path) && std::filesystem::file_size(path) >= 5 * 1024 * 1024) {
                const auto previous = std::filesystem::path(path.native() + L".previous");
                if (std::filesystem::exists(previous)) std::filesystem::remove(previous);
                std::filesystem::rename(path, previous);
            }
            destination = path.native();
        }
    } catch (const std::exception&) {}
    FILE* streams[2]{stdout, stderr};
    for (int index = 0; index < 2; ++index) {
        auto reopened = _wfreopen(destination.c_str(), L"a", streams[index]);
        if (!reopened) reopened = _wfreopen(L"NUL", L"a", streams[index]);
        if (reopened) {
            setvbuf(reopened, nullptr, _IONBF, 0);
            SetStdHandle(index == 0 ? STD_OUTPUT_HANDLE : STD_ERROR_HANDLE,
                reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(reopened))));
        }
    }
}

void insLogFormatted(std::string_view level, std::string_view message) noexcept {
    try {
        FILE* output = __acrt_iob_func(1);
        if (std::fwrite("[", 1, 1, output) != 1) return;
        if (std::fwrite(level.data(), 1, level.size(), output) != level.size()) return;
        if (std::fwrite("] ", 1, 2, output) != 2) return;
        if (std::fwrite(message.data(), 1, message.size(), output) != message.size()) return;
        std::fwrite("\n", 1, 1, output);
        std::fflush(output);
    } catch (const std::exception&) {}
}

std::string logging_detail::text(const std::vector<std::string>& values) {
    std::string result = "[";
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i) result += ", ";
        result += '"';
        for (const unsigned char value : values[i]) {
            switch (value) {
            case '\\': result += "\\\\"; break;
            case '"': result += "\\\""; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            case '\0': result += "\\0"; break;
            case '\a': result += "\\a"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\v': result += "\\v"; break;
            default:
                if (value < 32 || value == 127) {
                    constexpr char digits[] = "0123456789ABCDEF";
                    result += "\\x";
                    result += digits[value >> 4];
                    result += digits[value & 15];
                } else result += static_cast<char>(value);
                break;
            }
        }
        result += '"';
    }
    result += ']';
    return result;
}
}
