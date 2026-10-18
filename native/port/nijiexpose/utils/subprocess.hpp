#pragma once
// Windows branch of source/nijiexpose/utils/subprocess.d, in source order.
#include "dcompat/string.hpp"
#include <array>
#include <cstdio>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>
#include <optional>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef stdout
#undef stdout
#endif

namespace nijiexpose::utils {
template<bool readOutput = true, bool rawData = false>
class SubProcess {
protected:
    std::string executable;
    std::vector<std::string> args;
    int exitCode = -1;
    bool isRunning = false;
    PROCESS_INFORMATION pi{};
    STARTUPINFOW si{};
    HANDLE hStdOutRead = nullptr;
    HANDLE hStdOutWrite = nullptr;
    HANDLE hNullOutput = nullptr;
    std::array<char, 4096> buffer{};

    static void enforce(BOOL success, const char* message) {
        if (!success) throw std::runtime_error(message);
    }

public:
    virtual ~SubProcess()=default;
    using StdOutData = std::conditional_t<rawData, std::vector<std::uint8_t>, std::vector<std::string>>;
    StdOutData stdoutOutput;

    explicit SubProcess(std::string executable, std::vector<std::string> args = {})
        : executable(std::move(executable)), args(std::move(args)) {}

    virtual bool start() {
        SECURITY_ATTRIBUTES sa{};
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.bInheritHandle = TRUE;
        sa.lpSecurityDescriptor = nullptr;
        if constexpr (readOutput) {
            enforce(CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0), "Failed to create stdout pipe");
            enforce(SetHandleInformation(hStdOutRead, HANDLE_FLAG_INHERIT, 0), "Failed to set pipe handle info");
        } else {
            hNullOutput = CreateFileW(L"NUL", GENERIC_WRITE, FILE_SHARE_WRITE, &sa,
                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            enforce(hNullOutput != INVALID_HANDLE_VALUE, "Failed to open NUL output");
        }

        std::string quotedArgs;
        for (std::size_t i = 0; i < args.size(); ++i) {
            if (i) quotedArgs += ' ';
            quotedArgs += '"';
            quotedArgs += dcompat::fromStringz(args[i]);
            quotedArgs += '"';
        }
        const auto fullCmd = '"' + executable + '"' + (args.empty() ? "" : " " + quotedArgs);
        auto wideCmd = dcompat::toUTF16z(fullCmd);
        si.cb = sizeof(STARTUPINFOW);
        si.dwFlags = STARTF_USESTDHANDLES;
        if constexpr (readOutput) {
            si.hStdOutput = hStdOutWrite;
            si.hStdError = hStdOutWrite;
        } else {
            si.hStdOutput = hNullOutput;
            si.hStdError = hNullOutput;
        }
        si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        const DWORD flags = CREATE_NO_WINDOW;
        const BOOL success = CreateProcessW(nullptr, wideCmd.data(), nullptr, nullptr,
            true, flags, nullptr, nullptr, &si, &pi);
        Sleep(100);
        if (!success) return false;
        isRunning = true;
        if constexpr (readOutput) CloseHandle(hStdOutWrite);
        return true;
    }

    virtual void update() {
        bool stopping = false;
        if (isRunning) {
            DWORD code;
            if (GetExitCodeProcess(pi.hProcess, &code) && code != STILL_ACTIVE) {
                exitCode = static_cast<int>(code);
                isRunning = false;
                stopping = true;
            }
        }
        if constexpr (readOutput) {
            DWORD bytesAvailable = 0;
            while (PeekNamedPipe(hStdOutRead, nullptr, 0, nullptr, &bytesAvailable, nullptr) && bytesAvailable > 0) {
                DWORD bytesRead = 0;
                if (ReadFile(hStdOutRead, buffer.data(), static_cast<DWORD>(buffer.size()), &bytesRead, nullptr)) {
                    const std::string chunk(buffer.data(), bytesRead);
                    if constexpr (rawData) stdoutOutput.insert(stdoutOutput.end(), chunk.begin(), chunk.end());
                    else {
                        auto lines = dcompat::splitLines(chunk);
                        stdoutOutput.insert(stdoutOutput.end(), lines.begin(), lines.end());
                    }
                } else break;
            }
        }
        if (stopping) {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            if constexpr (readOutput) CloseHandle(hStdOutRead);
            else CloseHandle(hNullOutput);
        }
    }

    virtual void terminate() {
        if (isRunning) {
            isRunning = false;
            TerminateProcess(pi.hProcess, 1);
            WaitForSingleObject(pi.hProcess, INFINITE);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            if constexpr (readOutput) CloseHandle(hStdOutRead);
            else CloseHandle(hNullOutput);
        }
    }

    void rerun() {
        if (isRunning) terminate();
        start();
    }

    virtual int getExitCode() const { return exitCode; }
    virtual bool running() const { return isRunning; }
    StdOutData stdout() const requires(readOutput) { return stdoutOutput; }

    int execute() {
        if (!start()) return -1;
        while (running()) {
            update();
            Sleep(10);
        }
        return getExitCode();
    }
};
template<bool readOutput=true> class PythonProcess:public SubProcess<readOutput> {
    static inline thread_local std::optional<std::string> systemPythonPath;
    static std::optional<std::string> select(std::optional<std::string> path){if(path)return path;if(!systemPythonPath)systemPythonPath=detectPython();return systemPythonPath;}
    static std::vector<std::string> arguments(const std::optional<std::string>& script,std::vector<std::string> args){if(script)args.insert(args.begin(),*script);return args;}
    PythonProcess(std::optional<std::string> script,std::vector<std::string> args,std::optional<std::string> path,int):SubProcess<readOutput>(path.value_or(""),arguments(script,std::move(args))),pythonPath(std::move(path)){}
public:
    std::optional<std::string> pythonPath;
    PythonProcess(std::optional<std::string> script=std::nullopt,std::vector<std::string> args={},std::optional<std::string> path=std::nullopt):PythonProcess(std::move(script),std::move(args),select(std::move(path)),0){}
    bool start() override {if(!pythonPath)return false;return SubProcess<readOutput>::start();}
    static std::optional<std::string> detectPython(){for(auto cmd:{"python","python3","py"}){SubProcess<true> query(cmd,{"--version"});if(query.execute()==0)return std::string(cmd);}return std::nullopt;}
};
}
