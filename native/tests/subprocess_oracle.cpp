#include "nijiexpose/utils/subprocess.hpp"
#include <iostream>
#include <string>

namespace {
void hex(std::string_view text) {
    constexpr char digits[] = "0123456789abcdef";
    for (const unsigned char value : text) std::cout << digits[value >> 4] << digits[value & 15];
}
}
int main(int argc, char** argv) {
    if (argc < 3) return 2;
    const std::string mode = argv[1];
    using nijiexpose::utils::SubProcess;
    if (mode == "capture") {
        SubProcess<true> process(argv[2], {"arg with space", "quote\"tail", "end\\", "\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e", std::string("\0tail", 5)});
        std::cout << "code:" << process.execute() << '\n';
        for (const auto& line : process.stdout()) { hex(line); std::cout << "|\n"; }
    } else if (mode == "raw") {
        SubProcess<true, true> process(argv[2], {"arg with space"});
        std::cout << "code:" << process.execute() << '\n';
        const auto bytes = process.stdout();
        hex(std::string_view(reinterpret_cast<const char*>(bytes.data()), bytes.size()));
        std::cout << '\n';
    } else if (mode == "missing") {
        SubProcess<false> process("Z:\\nijiexpose-missing-fixture.exe");
        std::cout << "code:" << process.execute() << '\n';
    } else if (mode == "terminate") {
        SubProcess<false> process(argv[2], {"sleep"});
        std::cout << "start:" << std::boolalpha << process.start() << '\n';
        process.terminate();
        std::cout << "running:" << process.running() << ",code:" << process.getExitCode() << '\n';
    } else if (mode == "rerun") {
        SubProcess<false> process(argv[2]);
        std::cout << "code:" << process.execute() << '\n';
        process.rerun();
        while (process.running()) process.update();
        std::cout << "code:" << process.getExitCode() << '\n';
    } else {
        SubProcess<false> process(argv[2]);
        std::cout << "code:" << process.execute() << '\n';
    }
}
