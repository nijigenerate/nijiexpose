module nijiexpose.tracking.tracker;

import nijiexpose.utils.subprocess;
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
import requests;


class Tracker {
protected:

    DeviceInfo[] parseDeviceList(JSONValue jval) {
        if(jval.type != JSONType.ARRAY) {
            import std.stdio;
            writefln("Expected JSON Array. but get type of %s", jval.type);
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
    string trackerScriptName = "nijitrack.py";
    static enum ScriptDownloadSource = "https://github.com/nijigenerate/nijitrack/archive/refs/heads/main.zip";

    bool initialized = false;

    SubProcess!false process = null;
    SubProcess!true queryProcess = null;

    struct DeviceInfo {
        int id;
        string name;
    }
    DeviceInfo[] deviceList = null;

    this () {
        this.trackerPath = buildPath(thisExePath(), "nijitrack");
    }

    ~this() {
        debug(subprocess) import std.stdio;
        debug(subprocess) writefln("Terminate all");
        if (process !is null) process.terminate();
        if (queryProcess !is null) queryProcess.terminate();
        process = null;
        queryProcess = null;
    }

    string scriptPath() {
        return buildPath(trackerPath.fromStringz, trackerScriptName);
    }

    void serialize(S)(ref S serializer) {
        auto state = serializer.objectBegin;
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
        serializer.objectEnd(state);
    }
    
    SerdeException deserializeFromFghj(Fghj data) {
        if (!data["enabled"].isEmpty) data["enabled"].deserializeValue(enabled);
        if (!data["flipped"].isEmpty) data["flipped"].deserializeValue(flipped);
        if (!data["showWindow"].isEmpty) data["showWindow"].deserializeValue(showWindow);
        if (!data["device"].isEmpty) data["device"].deserializeValue(device);
        if (!data["hostname"].isEmpty) data["hostname"].deserializeValue(hostname);
        if (!data["port"].isEmpty) data["port"].deserializeValue(port);
        if (!data["trackerPath"].isEmpty) data["trackerPath"].deserializeValue(trackerPath);
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
        if (deviceList && !force) {
            return deviceList;
        }
        if (queryProcess is null) {
            queryProcess = new PythonProcess!true(scriptPath, ["--list-devices"]);
            queryProcess.start();
        } else {
            if (!queryProcess.running) {
                if (queryProcess.getExitCode() == 0) {
                    auto jsonValue = parseJSON(queryProcess.stdout.join("\n"));
                    deviceList = parseDeviceList(jsonValue);
                }
                queryProcess = null;
            }
        }
        return deviceList;
    }

    void restart() {
        if (device < 0) return;
        if (process !is null && process.running) process.terminate();
        string[] args = ["--device", device.to!string, "--osc-host", hostname, "--osc-port", port.to!string];
        if (showWindow) args ~= ["--show", "--show-video", "--show-wire"];
        if (flipped) args ~= ["--flip"];
        process = new PythonProcess!false(scriptPath, args);
        process.start();
    }

    void terminate() {
        if (process !is null && process.running) process.terminate();
        process = null;
    }

    SubProcess!true install() {
        import std.stdio: writefln;
        trackerPath = trackerPath.fromStringz;
        if (trackerPath is null) return null;
        auto data = getContent(ScriptDownloadSource).data;
        writefln("data length=%d", data.length);
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
            writefln("Wrote %s", destPath);
        }
        auto installProcess = new PythonProcess!true(null, ["-m", "pip", "install", trackerPath]);
        installProcess.start();
        while (installProcess.running()) {
            installProcess.update();
            import std.stdio;
            if (installProcess.stdoutOutput.length > 0)
                std.stdio.writeln(installProcess.stdoutOutput.join("\n"));
            installProcess.stdoutOutput.length = 0;
        }
        return installProcess;
    }

    void setup() {

    }

}

private {
    Tracker tracker;
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