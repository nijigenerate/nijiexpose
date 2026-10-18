#pragma once
#include <string>
#include <string_view>
#include <vector>

// Direct translations of the Phobos char[] operations used by the D app.
// Phobos is distributed under the Boost Software License (native/licenses/Boost.txt).
namespace nijiexpose::dcompat {
std::string_view fromStringz(std::string_view input);
std::string_view strip(std::string_view input);
std::vector<std::string> splitLines(std::string_view input);
std::wstring toUTF16z(std::string_view input);
std::string toUTF8(std::wstring_view input);
}
