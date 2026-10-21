module nijiexpose.tracking.ext.exvmc;
import ft.adaptor;
import ft.data;
import osc;
import std.conv : to;
import std.socket;
import inmath.linalg;
import std.uni: toUpper;

private {
    bool[int] keyStatus;
}

class ExVMCAdaptor : Adaptor {
private:
    Server server;
    ushort port = 39540;
    string bind = "0.0.0.0";

    bool gotDataFromFetch;

public:

    override 
    string getAdaptorName() {
        return "VMC Receiver";
    }

    override
    void start() {
        if ("port" in options) {
            port = to!ushort(options["port"]);
        }

        if ("address" in options) {
            bind = options["address"];
        }

        server = new Server(new InternetAddress(bind, port));
    }

    override
    bool isRunning() {
        return server !is null;
    }

    override
    void stop() {
        if (server) {
            server.close();
            server = null;
        }
    }

    override
    void poll() {
        if (!isRunning) return;
        import std.stdio;
        
        const(Message)[] msgs = server.popMessages();
        if (msgs.length > 0) {
            dataLossCounter = 0;
            gotDataFromFetch = true;

            foreach(const(Message) msg; msgs) {
                if (msg.addressPattern.length < 3) continue;
                if (msg.addressPattern[0].toString != "/VMC" && msg.addressPattern[1].toString != "/Ext") continue;
                switch(msg.addressPattern[2].toString) {
                    case "/Bone":
                        if (msg.addressPattern.length < 4) break;
                        if (msg.addressPattern[3].toString != "/Pos") break;
                        // msg form: /VMC/Ext/Bone/Pos/<Name> = [float x 7]
                        if (msg.addressPattern.length > 4) {
    
                            string pattern = msg.addressPattern[4].toString();
                            if (pattern.length > 1) {
                                
                                // Early escape for invalid bone seq length
                                if (msg.typeTags.length != 7) break;
                                
                                string boneName = pattern[$..1];
                                this.bones[boneName].position = vec3(
                                    msg.arg!float(0),
                                    msg.arg!float(1),
                                    msg.arg!float(2)
                                );
                                
                                // NOTE: the bones quaternion is modified here to match the output of the VTS Protocol
                                this.bones[boneName].rotation = quat(
                                    msg.arg!float(6), 
                                    -msg.arg!float(5), 
                                    msg.arg!float(3), 
                                    -msg.arg!float(4), 
                                );
                            }

                        // msg form: /VMC/Ext/Bone/Pos = [<Name>, float x 7]
                        } else {

                            // Early escape for invalid bone seq length
                            if (msg.typeTags.length != 8) break;

                            string boneName = msg.arg!string(0);
                            if (boneName !in bones) {
                                bones[boneName] = Bone(
                                    vec3.init,
                                    quat.identity
                                );
                            }

                            this.bones[boneName].position = vec3(
                                msg.arg!float(1),
                                msg.arg!float(2),
                                msg.arg!float(3)
                            );
                            
                            // NOTE: the bones quaternion is modified here to match the output of the VTS Protocol
                            this.bones[boneName].rotation = quat(
                                msg.arg!float(7), 
                                -msg.arg!float(6), 
                                msg.arg!float(4), 
                                -msg.arg!float(5), 
                            );
                        }
                        break;
                    case "/Blend":
                        if (msg.addressPattern.length > 3) {
                            string pattern = msg.addressPattern[3].toString();
                            switch (pattern) {
                                
                                // We don't use /Apply, so we just break out.
                                case "/Apply": break;
                                
                                case "/Val":
                                    // msg form: /VMC/Ext/Blend/Val  = [<Name>, float]
                                    // Expected VMC protocol case
                                    if (msg.typeTags.length == 2) {
                                        if(msg.arg!string(0).length > 0){
                                            this.blendshapes[msg.arg!string(0)] = msg.arg!float(1);
                                        }
                                    }
                                    // msg form: /VMC/Ext/Blend/Val/<Name> = [float]
                                    else if (msg.typeTags.length == 1) {
                                        if (msg.addressPattern.length != 4) break;
                                        pattern = msg.addressPattern[4].toString();
                                        // Avoid invalid string if name is an empty "/"".
                                        if (pattern.length > 1) {
                                            // Extension; for bones addressed via the pattern we need to handle it appropriately.
                                            this.blendshapes[pattern[1..$]] = msg.arg!float(0);
                                        }
                                    }

                                    break;
                                default: break;
                            }
                        }
                        break;
                    case "/Key":
                        auto active = msg.arg!int(0);
                        auto name = msg.arg!string(1);
                        auto keyCode = msg.arg!int(2);
                        if (active) {
                            keyStatus[keyCode.toUpper.to!int] = true;
                        } else {
                            keyStatus.remove(keyCode.toUpper.to!int);
                        }
                        break;
                    default: 
                        break;
                }
            }
        } else {
            dataLossCounter++;
            if (dataLossCounter > RECV_TIMEOUT) gotDataFromFetch = false;
        }
    }

    override
    bool isReceivingData() {
        return gotDataFromFetch;
    }

    override
    string[] getOptionNames() {
        return [
            "port", 
            "address"
        ];
    }
}

bool neIsEventOn(int keyCode) {
    return cast(bool)(keyCode.toUpper.to!int in keyStatus);
}