// Source: source/nijiexpose/utils/launchpath.d (see reference-sources.json).
#include "nijiexpose/utils/launchpath.hpp"
#include "dcompat/string.hpp"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

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
        const auto attributes = GetFileAttributesW(dcompat::toUTF16z(path).c_str());
        if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_DIRECTORY)) return false;
        return true;
    } catch (const std::exception&) {
        return false;
    }
}
}
