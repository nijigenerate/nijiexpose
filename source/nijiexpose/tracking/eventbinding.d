module nijiexpose.tracking.eventbinding;

import nijiexpose.tracking;
import nijiexpose.tracking.vspace;
import nijiexpose.scene;
import nijilive;
import nijilive.math.serialization;
import fghj;
import i18n;
import std.format;
import std.math.rounding : quantize;
import std.math : isFinite;
import std.array;
import std.uni: toUpper;
import bindbc.imgui;

string keyMapStr() {

    string keyMap(char keyCode) { return "\"%c\": ImGuiKey.%c".format(keyCode, keyCode); }
    string[] codes;
    for (char keyCode = 'A'; keyCode <= 'Z'; keyCode ++) {
        codes ~= keyMap(keyCode);
    }
    return "["~ codes.join(",") ~ "]";
}
private {
    ImGuiKey[string] keyMap_;

    ImGuiKey[string] keyMap() {
        if (keyMap_.length == 0)
            keyMap_ = mixin(keyMapStr());
        return keyMap_;
    }
}


class EventTrackingBinding : ITrackingBinding {
private:
public:
    this(TrackingBinding binding) {
        this.binding = binding;
        valueMap.length = 0;
    }

    TrackingBinding binding;
    float outVal = 0;

    /**
        Dampening level
    */
    int dampenLevel = 0;

    /**
        Expression (if in ExpressionBinding mode)
    */
    struct EventMap {
        SourceType type;
        string id;
        float value;
    }
    EventMap[] valueMap;

    override
    void serializeSelf(ref Serializer serializer) {
        serializer.putKey("dampenLevel");
        serializer.putValue(dampenLevel);
        serializer.putKey("value_map");
        auto state = serializer.listBegin;
            foreach (item; valueMap) {
                serializer.elemBegin;
                auto state2 = serializer.structBegin();
                serializer.putKey("type");
                serializer.serializeValue(item.type);
                serializer.putKey("id");
                serializer.putValue(item.id);
                serializer.putKey("value");
                serializer.putValue(item.value);
                serializer.structEnd(state2);
            }
        serializer.listEnd(state);
    }
    
    override
    SerdeException deserializeFromFghj(Fghj data) {
        valueMap.length = 0;
        if (!data["dampenLevel"].isEmpty) data["dampenLevel"].deserializeValue(dampenLevel);
        foreach (elem; data["value_map"].byElement) {
            EventMap item;
            elem["type"].deserializeValue(item.type);
            elem["id"].deserializeValue(item.id);
            elem["value"].deserializeValue(item.value);
            valueMap ~= item;
        }
        return null;
    }

    /**
        Sets the parameter out range to the default for the axis
    */
    void outRangeToDefault() {}

    /**
        Updates the parameter binding
    */
    bool update(out float result) {
        float src = outVal;
        bool valSet = false;
        foreach (item; valueMap) {
            if (item.id == "" || item.id is null) {
                if (!valSet) {
                    src = item.value;
                    valSet = true;
                }
            }
            if (item.id !in keyMap) continue;
            if (igIsKeyDown(keyMap[item.id.toUpper()]) || neIsEventOn(item.id[0])) {
                src = item.value;
                valSet = true;
                break;
            }
        }
        if (dampenLevel == 0) outVal = src;
        else {
            outVal = dampen(outVal, src, deltaTime(), cast(float)(11-dampenLevel));
            outVal = quantize(outVal, 0.0001);
        }
        result = binding.param.unmapAxis(binding.axis, outVal);
        return valSet;
    }
}