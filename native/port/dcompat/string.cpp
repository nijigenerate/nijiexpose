#include "dcompat/string.hpp"
#include <cstdint>
#include <stdexcept>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace nijiexpose::dcompat {
namespace {
bool isWhite(std::uint32_t c) {
    return (c >= 9 && c <= 13) || c == 32 || c == 0x85 || c == 0xa0 || c == 0x1680 ||
        (c >= 0x2000 && c <= 0x200a) || c == 0x2028 || c == 0x2029 || c == 0x202f ||
        c == 0x205f || c == 0x3000;
}

// Only a valid encoding can be whitespace on stripLeft's replacement-decoder path.
std::uint32_t decode(std::string_view input, std::size_t& index) {
    const auto first = static_cast<unsigned char>(input[index++]);
    if (first < 0x80) return first;
    int remaining;
    std::uint32_t value;
    std::uint32_t minimum;
    if (first >= 0xc2 && first <= 0xdf) { remaining = 1; value = first & 31; minimum = 0x80; }
    else if (first >= 0xe0 && first <= 0xef) { remaining = 2; value = first & 15; minimum = 0x800; }
    else if (first >= 0xf0 && first <= 0xf4) { remaining = 3; value = first & 7; minimum = 0x10000; }
    else return 0xfffd;
    if (index + remaining > input.size()) return 0xfffd;
    for (int n = 0; n < remaining; ++n) {
        const auto next = static_cast<unsigned char>(input[index]);
        if ((next & 0xc0) != 0x80) return 0xfffd;
        value = (value << 6) | (next & 63);
        ++index;
    }
    if (value < minimum || value > 0x10ffff || (value >= 0xd800 && value <= 0xdfff)) return 0xfffd;
    return value;
}

std::string_view stripLeft(std::string_view input) {
    std::size_t i = 0;
    for (; i < input.size(); ++i) {
        const auto c = static_cast<unsigned char>(input[i]);
        if (c >= 0x80) {
            input.remove_prefix(i);
            for (std::size_t index = 0; index < input.size();) {
                const auto saveIndex = index;
                if (!isWhite(decode(input, index))) return input.substr(saveIndex);
            }
            return input.substr(input.size());
        }
        if (!isWhite(c)) break;
    }
    return input.substr(i);
}

std::string_view stripRight(std::string_view str) {
    // Preserve Phobos' byte-level reverse path, including malformed UTF-8 behavior.
    std::size_t i = str.size();
    while (i--) {
        const auto cx = static_cast<unsigned char>(str[i]);
        if (cx <= 0x7f) {
            if (isWhite(cx)) continue;
            break;
        }
        if (i == 0 || (0xc0 & cx) != 0x80) break;
        const std::uint32_t d = 0x3f & cx;
        const auto c2 = static_cast<unsigned char>(str[i - 1]);
        if ((c2 & 0xe0) == 0xc0) {
            if (isWhite(d + (std::uint32_t(c2 & 0x1f) << 6))) { --i; continue; }
            break;
        }
        if (i == 1 || (c2 & 0xc0) != 0x80) break;
        const auto c3 = static_cast<unsigned char>(str[i - 2]);
        if ((c3 & 0xf0) == 0xe0 && isWhite(d + (std::uint32_t(c2 & 0x3f) << 6) + (std::uint32_t(c3 & 0xf) << 12))) {
            i -= 2;
            continue;
        }
        break;
    }
    return str.substr(0, i + 1);
}
}

std::string_view fromStringz(std::string_view input) {
    return input.substr(0, input.find('\0'));
}

std::string_view strip(std::string_view input) { return stripRight(stripLeft(input)); }

std::vector<std::string> splitLines(std::string_view s) {
    std::size_t iStart = 0;
    std::vector<std::string> retval;
    for (std::size_t i = 0; i < s.size(); ++i) {
        const auto c = static_cast<unsigned char>(s[i]);
        if (c == '\v' || c == '\f' || c == '\n' || c == '\r') {
            retval.emplace_back(s.substr(iStart, i - iStart));
            if (c == '\r' && i + 1 < s.size() && s[i + 1] == '\n') ++i;
            iStart = i + 1;
        } else if (c == 0xe2 && i + 2 < s.size() && static_cast<unsigned char>(s[i + 1]) == 0x80 &&
            (static_cast<unsigned char>(s[i + 2]) == 0xa8 || static_cast<unsigned char>(s[i + 2]) == 0xa9)) {
            retval.emplace_back(s.substr(iStart, i - iStart));
            i += 2;
            iStart = i + 1;
        } else if (c == 0xc2 && i + 1 < s.size() && static_cast<unsigned char>(s[i + 1]) == 0x85) {
            retval.emplace_back(s.substr(iStart, i - iStart));
            ++i;
            iStart = i + 1;
        }
    }
    if (iStart != s.size()) retval.emplace_back(s.substr(iStart));
    return retval;
}

std::wstring toUTF16z(std::string_view input) {
    if (input.empty()) return {};
    const int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, input.data(), static_cast<int>(input.size()), nullptr, 0);
    if (!size) throw std::runtime_error("Invalid UTF-8 sequence");
    std::wstring result(size, L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, input.data(), static_cast<int>(input.size()), result.data(), size);
    return result;
}
std::string toUTF8(std::wstring_view input) {
    if (input.empty()) return {};
    const int size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, input.data(), static_cast<int>(input.size()), nullptr, 0, nullptr, nullptr);
    if (!size) throw std::runtime_error("Invalid UTF-16 sequence");
    std::string result(size, '\0');
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, input.data(), static_cast<int>(input.size()), result.data(), size, nullptr, nullptr);
    return result;
}
}
