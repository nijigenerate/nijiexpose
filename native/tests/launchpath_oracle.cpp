#include "utils/launchpath.hpp"
#include "dcompat/string.hpp"
#include <iostream>
#include <string>

namespace {
void hex(std::string_view text) {
    constexpr char digits[] = "0123456789abcdef";
    for (const unsigned char value : text) std::cout << digits[value >> 4] << digits[value & 15];
}
}

int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.size() < 2) continue;
        std::string input;
        for (std::size_t i = 2; i + 1 < line.size(); i += 2)
            input += static_cast<char>(std::stoul(line.substr(i, 2), nullptr, 16));
        try {
            if (line[0] == 'A') std::cout << (nijiexpose::utils::executableFileAvailable(input) ? "1" : "0") << '\n';
            else if (line[0] == 'S') {
                for (const auto& value : nijiexpose::dcompat::splitLines(input)) { hex(value); std::cout << '|'; }
                std::cout << '\n';
            } else { hex(nijiexpose::utils::launchPath(input)); std::cout << '\n'; }
        } catch (const std::exception&) { std::cout << "EXCEPTION\n"; }
    }
}
