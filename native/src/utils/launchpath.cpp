// Source: legacy D: nijiexpose/utils/launchpath.d.
#include "utils/launchpath.hpp"
#include "dcompat/string.hpp"
#include <filesystem>
#ifndef _WIN32
#include <unistd.h>
#endif
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace nijiexpose::utils {
std::string launchPath(std::string_view input) {
    auto path = dcompat::strip(dcompat::fromStringz(input));
    if (path.length() >= 2 && path[0] == '"' && path[path.length() - 1] == '"') {
        path = path.substr(1, path.length() - 2);
    }
    return std::string(path);
}

bool executableFileAvailable(std::string_view input) {
    auto path = launchPath(input);
    try {
        if (!path.length()) return false;
        const auto candidate = dcompat::pathFromUTF8(path);
        if (!std::filesystem::is_regular_file(candidate)) return false;
#ifdef _WIN32
        const auto attributes = GetFileAttributesW(candidate.c_str());
        return attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
#else
        return access(candidate.c_str(), X_OK) == 0;
#endif
    } catch (const std::exception&) {
        return false;
    }
}
}
