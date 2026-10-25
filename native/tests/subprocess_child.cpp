#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>

int wmain(int argc, wchar_t** argv) {
    if (argc > 1 && std::wstring(argv[1]) == L"sleep") { Sleep(5000); return 9; }
    std::string text = "stdout\r\n";
    for (int i = 1; i < argc; ++i) {
        const std::wstring input(argv[i]);
        const int length = WideCharToMultiByte(CP_UTF8, 0, input.data(), static_cast<int>(input.size()), nullptr, 0, nullptr, nullptr);
        std::string value(length, '\0');
        WideCharToMultiByte(CP_UTF8, 0, input.data(), static_cast<int>(input.size()), value.data(), length, nullptr, nullptr);
        text += value + '\n';
    }
    text += std::string("raw\0data\n", 9);
    DWORD written;
    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), text.data(), static_cast<DWORD>(text.size()), &written, nullptr);
    const std::string error = "stderr\r\nlast";
    WriteFile(GetStdHandle(STD_ERROR_HANDLE), error.data(), static_cast<DWORD>(error.size()), &written, nullptr);
    return 7;
}
