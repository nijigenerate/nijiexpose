module nijiexpose.utils.subprocess;

import std.string;
import std.array;
import std.stdio;

version(Windows) {
    import core.sys.windows.windows;
    import std.utf : toUTF16z;

    class SubProcess(bool readOutput = true) {
        protected string executable;
        protected string[] args;
        protected int exitCode = -1;
        protected bool isRunning = false;
        protected PROCESS_INFORMATION pi;

        HANDLE hStdOutRead, hStdOutWrite;
        File stdoutFile;

        this(string executable, string[] args = []) {
            this.executable = executable;
            this.args = args.dup;
        }

        bool start() {
            SECURITY_ATTRIBUTES sa;
            sa.nLength = SECURITY_ATTRIBUTES.sizeof;
            sa.bInheritHandle = TRUE;

            // Create stdout pipe
            if (!CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0))
                return false;
            SetHandleInformation(hStdOutRead, HANDLE_FLAG_INHERIT, 0);

            auto fullCmd = format(`"%s"%s`,
                executable,
                args.length > 0 ? " " ~ args.join(" ") : ""
            );
            auto wideCmd = fullCmd.toUTF16z;

            STARTUPINFOW si = void;
            si.cb = STARTUPINFOW.sizeof;
            si.dwFlags = STARTF_USESTDHANDLES;
            si.hStdOutput = hStdOutWrite;
            si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
            si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

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

            CloseHandle(hStdOutWrite); // parent no longer writes

            if (success == FALSE)
                return false;

            stdoutFile = File(hStdOutRead, "rb");
            isRunning = true;
            return true;
        }

        void update() {
            if (isRunning) {
                DWORD code;
                if (GetExitCodeProcess(pi.hProcess, &code) && code != STILL_ACTIVE) {
                    exitCode = cast(int)code;
                    CloseHandle(pi.hProcess);
                    CloseHandle(pi.hThread);
                    isRunning = false;
                }
            }
        }

        void terminate() {
            if (isRunning) {
                TerminateProcess(pi.hProcess, 1);
                WaitForSingleObject(pi.hProcess, INFINITE);
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
                isRunning = false;
            }
        }

        void rerun() {
            if (isRunning) terminate();
            start();
        }

        int getExitCode() const { return exitCode; }
        bool running() const { return isRunning; }

        File stdoutHandle() { return stdoutFile; }
    }

} else {
    import std.process : pipeProcess, wait, tryWait, kill, ProcessPipes;
    import core.sys.posix.signal : SIGTERM;
    import core.sys.posix.fcntl : fcntl, F_GETFL, F_SETFL, O_NONBLOCK;
    import core.sys.posix.unistd : read;
    import core.stdc.errno : errno, EAGAIN;

    class SubProcess(bool readOutput = true) {
    protected:
        string executable;
        string[] args;
        int exitCode = -1;
        ProcessPipes pipes;
        static if (readOutput) {
            string[] stdoutOutput;
            enum BUF_SIZE = 4096;
            char[BUF_SIZE] buffer;
        }

    public:
        bool isRunning = false;

        this(string executable, string[] args = []) {
            this.executable = executable;
            this.args = args.dup;
        }

        bool start() {
            auto fullCmd = [executable] ~ args;
            pipes = pipeProcess(fullCmd);
            isRunning = true;
            writefln("exec %s", fullCmd, args);
            if (readOutput) {
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
//                    writefln("read=%d, exitCode=%d", bytesRead, exitCode);
                    if (bytesRead > 0) {
                        auto data = buffer[0 .. bytesRead].idup;
                        auto lines = data.splitLines();
                        stdoutOutput ~= lines;
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
                kill(pipes.pid, SIGTERM); // ← 正規の方法
                wait(pipes.pid);
                isRunning = false;
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
            string[] stdout() { return stdoutOutput; }
        }
    }
}

class PythonProcess(bool readOutput = true) : SubProcess!readOutput {
    this(string scriptPath, string[] scriptArgs = []) {
        super(detectPython(), [scriptPath] ~ scriptArgs);
    }

    static string detectPython() {
        version(Windows) {
            return "pythonw"; // Assume python is in PATH
        } else {
            import std.process : executeShell;
            try {
                auto res = executeShell("which python3");
                if (res.status == 0) return res.output.stripRight("\n");
            } catch (Throwable) {}
            return "python";
        }
    }
}