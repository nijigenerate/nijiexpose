module nijiexpose.tracking.tracker;

import nijiexpose.utils.subprocess;
import nijiui;
import fghj;
import std.json;
import std.exception;
import std.conv;
import std.array;
import std.file;


class Tracker {
protected:

    DeviceInfo[] parseDeviceList(JSONValue jval) {
        enforce(jval.type == JSONType.ARRAY, "Expected JSON array");

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
    string hostname = "localhost";
    uint port = 39540;
    int device =-1;
    string trackerPath;

    bool initialized = false;

    SubProcess!false process = null;
    SubProcess!true queryProcess = null;

    struct DeviceInfo {
        int id;
        string name;
    }
    DeviceInfo[] deviceList = null;

    this () {
        this.trackerPath = thisExePath() ~ "/nijitrack/nijitrack.py";
    }

    ~this() {
        import std.stdio;
        writefln("Terminate all");
        if (process !is null) process.terminate();
        if (queryProcess !is null) queryProcess.terminate();
        process = null;
        queryProcess = null;
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
            queryProcess = new PythonProcess!true(trackerPath, ["--list-devices"]);
//            queryProcess = new SubProcess!true("echo", ["[{\"id\": 0,\"name\": \"ELECOM 1MP Webcam: ELECOM 1MP W (usb-0000:05:00.3-2):\"}]"]);
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
        process = new PythonProcess!false(trackerPath, args);
        process.start();
    }

    void terminate() {
        if (process !is null && process.running) process.terminate();
        process = null;
    }

}

private {
    Tracker tracker;
}

ref Tracker ngTracker() {
    if (tracker is null) {
        tracker = new Tracker();
        inSettingsGet!Tracker("tracker", tracker);
    }
    return tracker;
}

void neInitTracker() {
    if (ngTracker.enabled)
        ngTracker.restart();
}