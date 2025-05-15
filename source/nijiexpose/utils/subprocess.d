module nijiexpose.utils.subprocess;

import std.string;
import std.array;
import std.stdio;
import core.thread.osthread;
import core.time;

version(Windows) {
    import core.sys.windows.windows;
    import std.utf : toUTF16z;
    import std.string : format, splitLines;
    import std.array : appender;
    import std.exception : enforce;
    import std.algorithm : map, joiner;

    class SubProcess(bool readOutput = true, bool rawData = false) {
    protected:
        string executable;
        string[] args;
        int exitCode = -1;
        bool isRunning = false;
        PROCESS_INFORMATION pi;
        STARTUPINFOW si = void;

        HANDLE hStdOutRead;
        HANDLE hStdOutWrite;
        HANDLE hStdErrorWrite;
        HANDLE hStdErrorRead;

        static if (readOutput) {
            char[4096] buffer;
        }

    public:
        static if (readOutput) {
            static if (rawData) {
                alias StdOutData = ubyte[];
            } else {
                alias StdOutData = string[];
            }
            StdOutData stdoutOutput;
        }
        this(string executable, string[] args = []) {
            this.executable = executable;
            this.args = args.dup;
        }

        bool start() {
            SECURITY_ATTRIBUTES sa;
            sa.nLength = SECURITY_ATTRIBUTES.sizeof;
            sa.bInheritHandle = TRUE;
            sa.lpSecurityDescriptor = null;

            // Create stdout pipe
            enforce(CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0), "Failed to create stdout pipe");
            enforce(CreatePipe(&hStdErrorRead, &hStdErrorWrite, &sa, 0), "Failed to create stdout pipe");
            enforce(SetHandleInformation(hStdOutRead, HANDLE_FLAG_INHERIT, 0), "Failed to set pipe handle info");

            auto quotedArgs = args.map!(a => `"` ~ a.fromStringz ~ `"`).join(" ");
            auto fullCmd = format(`"%s"%s`, executable, args.length > 0 ? " " ~ quotedArgs : "");
            auto wideCmd = fullCmd.toUTF16z;

            debug(subprocess) { writefln("cmd: [%s]", fullCmd); }
            si.cb = STARTUPINFOW.sizeof;
            si.dwFlags = STARTF_USESTDHANDLES;
            si.hStdOutput = hStdOutWrite;
            si.hStdError  = hStdErrorWrite;
            si.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);

            DWORD flags = CREATE_NO_WINDOW;

            BOOL success = CreateProcessW(
                null,
                cast(LPWSTR)wideCmd,
                null, null,
                true, // inherit handles
                flags,
                null, null,
                &si, &pi
            );
            Sleep(100);

            if (!success)
                return false;

            isRunning = true;
            CloseHandle(hStdOutWrite);
            CloseHandle(hStdErrorWrite);

            return true;
        }

        void update() {
            bool stopping = false;
            if (isRunning) {
                DWORD code;
                if (GetExitCodeProcess(pi.hProcess, &code) && code != STILL_ACTIVE) {
                    exitCode = cast(int)code;
                    isRunning = false;
                    stopping = true;
                }
            }

            static if (readOutput) {
                DWORD bytesAvailable = 0;
                while (PeekNamedPipe(hStdOutRead, null, 0, null, &bytesAvailable, null) && bytesAvailable > 0) {
                    DWORD bytesRead = 0;
                    if (ReadFile(hStdOutRead, buffer.ptr, cast(DWORD)buffer.length, &bytesRead, null)) {
                        auto chunk = buffer[0 .. bytesRead].idup;
                        if (rawData) {
                            stdoutOutput ~= chunk;
                        } else {
                            auto lines = chunk.splitLines();
                            stdoutOutput ~= lines;
                        }
                    } else {
                        break;
                    }
                }
                debug (subprocess) if (stopping) writefln("stdoutOutput=%s", stdoutOutput);
            }

            if (stopping) {
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
                CloseHandle(hStdOutRead);
            }
        }

        void terminate() {
            if (isRunning) {
                isRunning = false;
                TerminateProcess(pi.hProcess, 1);
                WaitForSingleObject(pi.hProcess, INFINITE);
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
                CloseHandle(hStdOutRead);
            }
        }

        void rerun() {
            if (isRunning) terminate();
            start();
        }

        int getExitCode() const { return exitCode; }
        bool running() const { return isRunning; }

        static if (readOutput) {
            StdOutData stdout() { return stdoutOutput; } 
        }

        int execute() {
            if (!start()) {
                return -1; // Should be determined
            }
            while (running()) {
                update();
                Thread.sleep(dur!"msecs"(10));
            }
            return getExitCode();
        }
    }
} else {
    import std.process : pipeProcess, wait, tryWait, kill, ProcessPipes;
    import core.sys.posix.signal : SIGTERM;
    import core.sys.posix.fcntl : fcntl, F_GETFL, F_SETFL, O_NONBLOCK;
    import core.sys.posix.unistd : read;
    import core.stdc.errno : errno, EAGAIN;

    class SubProcess(bool readOutput = true, bool rawData = false) {
    protected:
        string executable;
        string[] args;
        int exitCode = -1;
        ProcessPipes pipes;
        static if (readOutput) {
            enum BUF_SIZE = 4096;
            char[BUF_SIZE] buffer;
        }

    public:
        bool isRunning = false;
        static if (readOutput) {
            static if (rawData) {
                alias StdOutData = ubyte[];
            } else {
                alias StdOutData = string[];
            }
            StdOutData stdoutOutput;
        }

        this(string executable, string[] args = []) {
            this.executable = executable;
            this.args = args.dup;
        }

        bool start() {
            auto fullCmd = [executable] ~ args;
            pipes = pipeProcess(fullCmd);
            isRunning = true;
            import std.stdio;
            debug(subprocess) writefln("exec %s", fullCmd, args);
            static if (readOutput) {
                int fd = pipes.stdout.fileno;
                auto flags = fcntl(fd, F_GETFL, 0);
                fcntl(fd, F_SETFL, flags | O_NONBLOCK);
            }
            return true;
        }

        void update() {
            auto result = tryWait(pipes.pid);
            if (result.terminated) {
                exitCode = result.status;
                isRunning = false;
            }
            static if (readOutput) {
                while (true) {
                    int fd = stdoutHandle.fileno;
                    int bytesRead = cast(int)read(fd, buffer.ptr, BUF_SIZE);
                    if (bytesRead > 0) {
                        auto data = buffer[0 .. bytesRead].idup;
                        static if (rawData) {
                            stdoutOutput ~= data;
                        } else {
                            auto lines = data.splitLines();
                            stdoutOutput ~= lines;
                        }
                    } else if (bytesRead == -1 && errno == EAGAIN) {
                        break;
                    } else {
                        break;
                    }
                }               
            }
        }

        void terminate() {
            if (isRunning) {
                isRunning = false;
                kill(pipes.pid, SIGTERM); // ← 正規の方法
                wait(pipes.pid);
            }
        }

        void rerun() {
            if (isRunning) terminate();
            start();
        }

        int getExitCode() const { return exitCode; }
        bool running() const { return isRunning; }
        File stdoutHandle() { return pipes.stdout; }
        static if (readOutput) {
            StdOutData stdout() { return stdoutOutput; }
        }

        int execute() {
            if (!start()) {
                return -1; // Should be determined
            }
            while (running()) {
                update();
                Thread.sleep(dur!"msecs"(10));
            }
            return getExitCode();
        }
    }
}

class PythonProcess(bool readOutput = true) : SubProcess!readOutput {
    static string systemPythonPath = null;
    string pythonPath = null;
    this(string scriptPath = null, string[] scriptArgs = [], string pythonPath = null) {
        if (pythonPath !is null) { 
            this.pythonPath = pythonPath;
        } else if (systemPythonPath !is null) {
            this.pythonPath = systemPythonPath;
        } else {
            systemPythonPath = detectPython();
            this.pythonPath = systemPythonPath;
        }
        super(this.pythonPath, ((scriptPath !is null)? [scriptPath]:[]) ~ scriptArgs);
    }

    override
    bool start() {
        if (pythonPath is null) return false;
        return super.start();
    }

    static string detectPython() {
        auto queryCommands = ["python", "python3", "py"];
        version(Windows) {
            foreach (cmd; queryCommands) {
                auto queryProc = new SubProcess!true(cmd, ["--version"]);
                if (queryProc.execute() == 0) {
                    return cmd;
                }
            }
            return null;
        } else {
            import std.process : executeShell;
            foreach (cmd; queryCommands) {
                try {
                    auto res = executeShell("which "~cmd);
                    if (res.status == 0) return res.output.stripRight("\n");
                } catch (Throwable) {}
            }
            return null;
        }
    }

    
}