#include "log.hpp"
#include <cstdio>
int main() {
    using namespace nijiexpose;
    insInitLogging();
    insLogDebug("debug");
    insLogInfo("hello %s", "\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e");
    insLogWarn("two %s %s", "strings", "test");
    insLogErr("error %s", 42);
    insLogInfo("args=%s", std::vector<std::string>{"a", "line\nnext", "tab\tx", "quote\"", std::string("nul\0tail", 8), "\\"});
    std::string controls;
    for (int i = 0; i < 32; ++i) controls += static_cast<char>(i);
    insLogInfo("controls=%s", std::vector<std::string>{controls});
    insLogInfo("broken %s");
    insLogInfo("percent %%");
    std::puts("stdout dependency");
    std::fputs("stderr dependency\n", stderr);
}
