module nijiexpose.tracking.tracker;

import nijiexpose.utils.subprocess;
import nijiexpose.utils.launchpath : launchPath, executableFileAvailable;
import i18n : _;
import nijiexpose.log;
import nijiexpose.scene;
import nijiexpose.tracking.ext.exvmc;
import nijiexpose.tracking.vspace;
import ft.adaptor;
import ft.data;
import nijiui;
import fghj;
import std.json;
import std.exception;
import std.conv;
import std.array;
import std.file;
import std.string;
import std.path;
import std.zip;

class Tracker {
protected:

    DeviceInfo[] parseDeviceList(JSONValue jval) {
        if(jval.type != JSONType.ARRAY) {
            return [];
        }

        DeviceInfo[] devices;
        foreach (item; jval.array) {
            auto obj = item.object;
            devices ~= DeviceInfo(
                cast(int)obj["id"].integer,
                obj["name"].str
            );
        }
        return devices;
    }

public:
    bool enabled = false;
    bool flipped = true;
    bool showWindow = false;
    string hostname = "127.0.0.1";
    uint port = 39540;
    int device =-1;
    string trackerPath;
    bool nativeExecutable = false;
    string executablePath;
    string pythonExecutablePath;
    private string deviceQueryKey;
    private bool queryFailed;
    string launchError;
    string trackerScriptName = "nijitrack.py";
    static enum ScriptDownloadSource = "https://github.com/nijigenerate/nijitrack/archive/refs/heads/main.zip";

    bool initialized = false;

    SubProcess!false process = null;
    SubProcess!true queryProcess = null;
    SubProcess!true installProcess = null;

    struct DeviceInfo {
        int id;
        string name;
    }
    DeviceInfo[] deviceList = null;

    this () {
        this.trackerPath = buildPath(thisExePath().dirName, "nijitrack");
        version (Windows) executablePath = buildPath(trackerPath, "nijitrack.exe");
        else executablePath = buildPath(trackerPath, "nijitrack");
    }

    string scriptPath() {
        return buildPath(launchPath(trackerPath), trackerScriptName);
    }

    string executableFilePath() { return launchPath(executablePath); }

    string launchConfigurationKey() {
        return nativeExecutable ? "native\n" ~ executableFilePath
            : "python\n" ~ scriptPath ~ "\n" ~ launchPath(pythonExecutablePath);
    }

    string validationError() {
        if (nativeExecutable) {
            if (!executableFileAvailable(executableFilePath))
                return _("Tracker executable is missing or cannot be executed.");
        } else {
            if (launchPath(pythonExecutablePath).length && !executableFileAvailable(pythonExecutablePath))
                return _("Python executable is missing or cannot be executed.");
            try {
                if (!scriptPath.exists || !scriptPath.isFile)
                    return _("Tracker script was not found.");
            } catch (Exception) {
                return _("Tracker script was not found.");
            }
        }
        return null;
    }

    void invalidateDeviceQuery(bool resetSelection = true) {
        if (queryProcess !is null && queryProcess.running) queryProcess.terminate();
        queryProcess = null;
        deviceList = null;
        if (resetSelection) device = -1;
        deviceQueryKey = launchConfigurationKey;
        queryFailed = false;
        launchError = null;
    }

    string pythonPath() {
        if (launchPath(pythonExecutablePath).length) return launchPath(pythonExecutablePath);
        version(Windows) {
            auto bundledPython = buildPath(launchPath(trackerPath), ".venv", "Scripts", "python.exe");
        } else {
            auto bundledPython = buildPath(launchPath(trackerPath), ".venv", "bin", "python");
        }
        if (bundledPython.exists) return bundledPython;
        return PythonProcess!false.detectPython();
    }

    void serialize(S)(ref S serializer) {
        auto state = serializer.structBegin;
            serializer.putKey("enabled");
            serializer.putValue(enabled);
            serializer.putKey("flipped");
            serializer.putValue(flipped);
            serializer.putKey("showWindow");
            serializer.putValue(showWindow);
            serializer.putKey("device");
            serializer.putValue(device);
            serializer.putKey("hostname");
            serializer.putValue(hostname);
            serializer.putKey("port");
            serializer.putValue(port);
            serializer.putKey("trackerPath");
            serializer.putValue(trackerPath);
            serializer.putKey("nativeExecutable");
            serializer.putValue(nativeExecutable);
            serializer.putKey("executablePath");
            serializer.putValue(executablePath);
            serializer.putKey("pythonExecutablePath");
            serializer.putValue(pythonExecutablePath);
        serializer.structEnd(state);
    }
    
    SerdeException deserializeFromFghj(Fghj data) {
        if (!data["enabled"].isEmpty) data["enabled"].deserializeValue(enabled);
        if (!data["flipped"].isEmpty) data["flipped"].deserializeValue(flipped);
        if (!data["showWindow"].isEmpty) data["showWindow"].deserializeValue(showWindow);
        if (!data["device"].isEmpty) data["device"].deserializeValue(device);
        if (!data["hostname"].isEmpty) data["hostname"].deserializeValue(hostname);
        if (!data["port"].isEmpty) data["port"].deserializeValue(port);
        if (!data["trackerPath"].isEmpty) data["trackerPath"].deserializeValue(trackerPath);
        if (!data["nativeExecutable"].isEmpty) data["nativeExecutable"].deserializeValue(nativeExecutable);
        if (!data["executablePath"].isEmpty) data["executablePath"].deserializeValue(executablePath);
        if (!data["pythonExecutablePath"].isEmpty) data["pythonExecutablePath"].deserializeValue(pythonExecutablePath);
        hostname = hostname.fromStringz;
        trackerPath = trackerPath.fromStringz;
        initialized = true;
        return null;
    }

    void update() {
        if (process !is null) {
            process.update();
        }
        if (queryProcess !is null) {
            queryProcess.update();
        }
    }

    bool running() {
        return process && process.running;
    }

    DeviceInfo[] listDevices(bool force = false)() {
        auto key = launchConfigurationKey;
        if (key != deviceQueryKey) {
            invalidateDeviceQuery(deviceQueryKey.length > 0);
        }
        if (force) { queryFailed = false; launchError = null; }
        if (queryFailed) return deviceList;
        launchError = validationError;
        if (launchError.length) return null;
        if (deviceList && !force) {
            return deviceList;
        }
        try {
            if (queryProcess is null) {
                queryProcess = createProcess!true(["--list-devices"]);
                enforce(queryProcess.start(), _("Could not start the tracker."));
            } else {
                if (!queryProcess.running) {
                    if (queryProcess.getExitCode() == 0) {
                        auto jsonValue = parseJSON(queryProcess.stdout.join("\n"));
                        deviceList = parseDeviceList(jsonValue);
                        import std.algorithm : any;
                        if (!deviceList.any!(entry => entry.id == device)) device = -1;
                    } else {
                        throw new Exception(_("Could not list tracker cameras."));
                    }
                    queryProcess = null;
                }
            }
        } catch (Exception ex) {
            launchError = ex.msg;
            queryFailed = true;
            queryProcess = null;
            insLogErr("Camera query failed: %s", ex.msg);
        }
        return deviceList;
    }

    void restart() {
        launchError = validationError;
        if (launchError.length) {
            insLogErr("Tracker launch rejected: %s", launchError);
            return;
        }
        if (device < 0) return;
        if (process !is null && process.running) process.terminate();
        string[] args = ["--device", device.to!string, "--osc-host", hostname, "--osc-port", port.to!string];
        if (showWindow) args ~= ["--show", "--show-video", "--show-wire"];
        if (flipped) args ~= ["--flip"];
        try {
            process = createProcess!false(args);
            enforce(process.start(), _("Could not start the tracker."));
        } catch (Exception ex) {
            launchError = ex.msg;
            process = null;
            insLogErr("Tracker launch failed: %s", ex.msg);
        }
    }

    SubProcess!capture createProcess(bool capture)(string[] args) {
        auto problem = validationError;
        enforce(!problem.length, problem);
        if (nativeExecutable) {
            enforce(executableFilePath.length > 0, "Tracker executable path is empty");
            return new SubProcess!capture(executableFilePath, args);
        }
        auto interpreter = pythonPath;
        enforce(interpreter.length > 0, _("Python executable was not found."));
        return new PythonProcess!capture(scriptPath, args, interpreter);
    }

    void terminate() {
        if (process !is null && process.running) process.terminate();
        if (queryProcess !is null && queryProcess.running) queryProcess.terminate();
        if (installProcess !is null && installProcess.running) installProcess.terminate();

        process = null;
        queryProcess = null;
    }

    SubProcess!true install() {
        enforce(!nativeExecutable, "Automatic installation is only available for Python trackers");
        import std.stdio: writefln;
        trackerPath = launchPath(trackerPath);
        if (trackerPath is null) return null;

        string url = ScriptDownloadSource;
        ubyte[] data;
        version(OSX) {
            auto downloadProcess = new SubProcess!(true, true)("curl", ["-o", "-", "-L", url]);
            int ret = downloadProcess.execute();
            data = downloadProcess.stdoutOutput;
        } else {
            import vibe.http.client;
            import vibe.stream.operations;
            for (int redirect = 0; redirect <= 5; ++redirect) {
                insLogInfo("Download %s".format(url));
                auto res = requestHTTP(url);
                if (res.statusCode >= 300 && res.statusCode < 400) {
                    auto loc = res.headers.get("Location");
                    if (loc.length == 0) {
                        throw new Exception("Redirect (HTTP 302) received but no Location header.");
                    }
                    insLogInfo("Redirecting to: %s".format(loc));
                    url = loc;
                    continue;
                } else if (res.statusCode != 200) {
                    throw new Exception("HTTP error: " ~ to!string(res.statusCode));
                } else {
                    data = res.bodyReader.readAll();
                }
            }
        }
        auto zip = new ZipArchive(data);
        foreach (name, member; zip.directory) {
            auto parts = pathSplitter(name).array;

            if (parts.length <= 1) continue;

            auto relativePath = buildPath(parts[1 .. $]);

            if (relativePath.length == 0 || relativePath.endsWith("/")) continue;

            auto destPath = buildPath(trackerPath, relativePath);
            mkdirRecurse(dirName(destPath));
            zip.expand(member);
            write(destPath, member.expandedData);
            insLogInfo("Wrote %s".format(destPath));
        }
        installProcess = new PythonProcess!true(null, ["-m", "pip", "install", trackerPath], pythonPath);
        installProcess.start();
        return installProcess;
    }

    void setupVSpace() {
        auto space = insScene().space;
        foreach (zone; space.getZones()) {
            foreach (source; zone.sources) {
                if (source.getAdaptorName() == "VMC Receiver") {
                    if (source.getOptions()["port"] == "39540") return;
                }
            }
        }
        VirtualSpaceZone zone = new VirtualSpaceZone("Default");
        insScene.space.addZone(zone);
        zone.sources.length ++;
        auto source = new ExVMCAdaptor();
        source.setOptions(["port": "39540"]);
        source.start();
        zone.sources[$-1] = source;
    }

}

private {
    Tracker tracker;
}

unittest {
    // Invalid persisted enabled settings must never prevent startup.
    auto t = new Tracker();
    assert(dirName(t.executableFilePath) == t.trackerPath);
    assert(dirName(t.scriptPath) == t.trackerPath);
    t.nativeExecutable = true;
    t.executablePath = null;
    t.enabled = true;
    t.device = 3;
    t.restart();
    assert(t.launchError.length && !t.running);
    t.listDevices();
    assert(t.launchError.length && t.queryProcess is null);

    import std.uuid : randomUUID;
    auto fixture = buildPath(tempDir(), "nijiexpose-tracker-" ~ randomUUID().toString ~ ".exe");
    write(fixture, "not an executable");
    scope(exit) remove(fixture);
    t.executablePath = fixture;
    t.listDevices();
    assert(t.device == -1); // launch configuration changed
    assert(t.launchError.length);
    auto failure = t.launchError;
    t.listDevices();
    assert(t.launchError == failure && t.queryProcess is null);
    t.device = 3;
    t.restart(); // existing but invalid executable: process creation fails safely
    assert(t.launchError.length && !t.running);

    // A valid script does not make an invalid explicit interpreter available.
    t.nativeExecutable = false;
    t.trackerPath = dirName(fixture);
    t.trackerScriptName = baseName(fixture);
    t.pythonExecutablePath = fixture ~ ".missing";
    assert(t.validationError.length);
    t.restart();
    assert(t.launchError.length && !t.running);
    t.listDevices();
    assert(t.launchError.length && t.queryProcess is null && t.device == -1);
}

ref Tracker neTracker() {
    if (tracker is null) {
        tracker = new Tracker();
        inSettingsGet!Tracker("tracker", tracker);
    }
    return tracker;
}

void neInitTracker() {
    if (neTracker.enabled)
        neTracker.restart();
}

void neShutdownTracker() {
    if (neTracker.running) {
        neTracker.terminate();
    }
}
