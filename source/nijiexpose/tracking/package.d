/*
    Copyright © 2022, nijigenerate Project
    Distributed under the 2-Clause BSD License, see LICENSE file.
    
    Authors: Luna Nielsen
*/
module nijiexpose.tracking;
import nijiexpose.tracking.expr;
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

/**
    Binding Type
*/
enum BindingType {
    /**
        A binding where the base source is blended via
        in/out ratios
    */
    RatioBinding,

    /**
        A binding in which math expressions are used to
        blend between the sources in the VirtualSpace zone.
    */
    ExpressionBinding,

    /**
        A binding triggered by event.
    */
    EventBinding,

    /**
        Binding controlled from an external source.
        Eg. over the internet or from a plugin.
    */
    External
}

/**
    Source type
*/
enum SourceType {
    /**
        The source is a blendshape
    */
    Blendshape,

    /**
        Source is the X position of a bone
    */
    BonePosX,

    /**
        Source is the Y position of a bone
    */
    BonePosY,

    /**
        Source is the Y position of a bone
    */
    BonePosZ,

    /**
        Source is the roll of a bone
    */
    BoneRotRoll,

    /**
        Source is the pitch of a bone
    */
    BoneRotPitch,

    /**
        Source is the yaw of a bone
    */
    BoneRotYaw,

    /** 
     * Source is the key press
     */
    KeyPress,
}

/**
    Tracking Binding 
*/

alias Serializer = JsonSerializer!("", void delegate(const(char)[]) pure nothrow @safe);

interface ITrackingBinding {
    void serializeSelf(ref Serializer serializer);
    SerdeException deserializeFromFghj(Fghj data);
    void outRangeToDefault();
    void update();
}

class TrackingBinding {
protected:
    // UUID of param to map to
    uint paramUUID;

    // Sum of weighted plugin values
    float sum = 0;

    // Combined value of weights
    float weights = 0;

public:
    ITrackingBinding delegated;

    /// Last input value
    float inVal = 0;

    /// Last output value
    float outVal = 0;

    /**
        Display name for the binding
    */
    string name;

    /**
        Name of the source blendshape or bone
    */
    string sourceName;

    /**
        Display Name of the source blendshape or bone
    */
    string sourceDisplayName;

    /**
        The type of the binding
    */
    BindingType type_;

    /**
        The type of the tracking source
    */
    SourceType sourceType;

    /**
        The nijilive parameter it should apply to
    */
    Parameter param;

    /**
        Weights the user has set for each plugin
    */
    float[string] pluginWeights;

    /**
        The axis to apply the binding to
    */
    int axis = 0;

    /**
        Dampening level
    */
    int dampenLevel = 0;

    BindingType type() { return type_; }
    void type(BindingType value) {
        type_ = value;
        switch (type_) {
            case BindingType.RatioBinding:
                delegated = new RatioTrackingBinding(this);
                break;
            case BindingType.ExpressionBinding:
                delegated = new ExpressionTrackingBinding(this);
                break;
            case BindingType.EventBinding:
                delegated = new EventTrackingBinding(this);
                break;
            default:
                break;
        }
        ///
    }

    void serialize(S)(ref S serializer) {
        auto state = serializer.objectBegin;
            serializer.putKey("name");
            serializer.putValue(name);
            serializer.putKey("sourceName");
            serializer.putValue(sourceName);
            serializer.putKey("sourceDisplayName");
            serializer.putValue(sourceDisplayName);
            serializer.putKey("sourceType");
            serializer.serializeValue(sourceType);
            serializer.putKey("bindingType");
            serializer.serializeValue(type_);
            serializer.putKey("param");
            serializer.serializeValue(param.uuid);
            serializer.putKey("axis");
            serializer.putValue(axis);
            serializer.putKey("dampenLevel");
            serializer.putValue(dampenLevel);

            if (delegated)
                delegated.serializeSelf(serializer);

        serializer.objectEnd(state);
    }
    
    SerdeException deserializeFromFghj(Fghj data) {
        data["name"].deserializeValue(name);
        data["sourceName"].deserializeValue(sourceName);
        data["sourceType"].deserializeValue(sourceType);
        data["bindingType"].deserializeValue(type_);
        type = type_;
        data["param"].deserializeValue(paramUUID);
        if (!data["axis"].isEmpty) data["axis"].deserializeValue(axis);
        if (!data["dampenLevel"].isEmpty) data["dampenLevel"].deserializeValue(dampenLevel);

        if (delegated) {
            delegated.deserializeFromFghj(data);
        }
        this.createSourceDisplayName();
        
        return null;
    }

    /**
        Sets the parameter out range to the default for the axis
    */
    void outRangeToDefault() {
        if (delegated)
            delegated.outRangeToDefault();
    }

    /**
        Finalizes the tracking binding, if possible.
        Returns true on success.
        Returns false if the parameter does not exist.
    */
    bool finalize(ref Puppet puppet) {
        param = puppet.findParameter(paramUUID);
        return param !is null;
    }

    /**
        Updates the parameter binding
    */
    void update() {
        if (delegated)
            delegated.update();
    }
    
    /**
        Submit value for late update application
    */
    void submit(string plugin, float value) {
        if (plugin !in pluginWeights)
            pluginWeights[plugin] = 1;
        
        sum += value*pluginWeights[plugin];
        weights += pluginWeights[plugin];
    }

    /**
        Apply all the weighted plugin values
    */
    void lateUpdate() {
        if (weights > 0) param.value.vector[axis] += round(sum / weights);
    }

    void createSourceDisplayName() {
        switch(sourceType) {
            case SourceType.Blendshape:
                sourceDisplayName = sourceName;
                break;
            case SourceType.BonePosX:
                sourceDisplayName = _("%s (X)").format(sourceName);
                break;
            case SourceType.BonePosY:
                sourceDisplayName = _("%s (Y)").format(sourceName);
                break;
            case SourceType.BonePosZ:
                sourceDisplayName = _("%s (Z)").format(sourceName);
                break;
            case SourceType.BoneRotRoll:
                sourceDisplayName = _("%s (Roll)").format(sourceName);
                break;
            case SourceType.BoneRotPitch:
                sourceDisplayName = _("%s (Pitch)").format(sourceName);
                break;
            case SourceType.BoneRotYaw:
                sourceDisplayName = _("%s (Yaw)").format(sourceName);
                break;
            case SourceType.KeyPress:
                sourceDisplayName = _("%s (Key)").format(sourceName);
                break;
            default: assert(0);    
        }
    }
}

/**
    Ratio Tracking Binding 
*/
class RatioTrackingBinding : ITrackingBinding {
private:
    TrackingBinding binding;
    /**
        Maps an input value to an offset (0.0->1.0)
    */
    float mapValue(float value, float min, float max) {
        float range = max - min;
        float tmp = (value - min);
        float off = tmp / range;
        return clamp(off, 0, 1);
    }

    /**
        Maps an offset (0.0->1.0) to a value
    */
    float unmapValue(float offset, float min, float max) {
        float range = max - min;
        return (range * offset) + min;
    }


public:
    this(TrackingBinding binding) { this.binding = binding; }
    /// Ratio for input
    vec2 inRange = vec2(0, 1);

    /// Ratio for output
    vec2 outRange = vec2(0, 1);

    /**
        Whether to inverse the binding
    */
    bool inverse;

    override
    void serializeSelf(ref Serializer serializer) {
        serializer.putKey("inverse");
        serializer.putValue(inverse);

        serializer.putKey("inRange");
        inRange.serialize(serializer);
        serializer.putKey("outRange");
        outRange.serialize(serializer);
    }
    
    override
    SerdeException deserializeFromFghj(Fghj data) {
        data["inverse"].deserializeValue(inverse);
        inRange.deserialize(data["inRange"]);
        outRange.deserialize(data["outRange"]);
        return null;
    }

    /**
        Sets the parameter out range to the default for the axis
    */
    void outRangeToDefault() {
        outRange = vec2(binding.param.min.vector[binding.axis], binding.param.max.vector[binding.axis]);
    }

    /**
        Updates the parameter binding
    */
    void update() {
        if (binding.sourceName.length == 0) {
            binding.param.value.vector[binding.axis] = binding.param.defaults.vector[binding.axis];
            return;
        }

        float src = 0;
        if (insScene.space.currentZone) {
            switch(binding.sourceType) {

                case SourceType.Blendshape:
                    src = insScene.space.currentZone.getBlendshapeFor(binding.sourceName);
                    break;

                case SourceType.BonePosX:
                    src = insScene.space.currentZone.getBoneFor(binding.sourceName).position.x;
                    break;

                case SourceType.BonePosY:
                    src = insScene.space.currentZone.getBoneFor(binding.sourceName).position.y;
                    break;

                case SourceType.BonePosZ:
                    src = insScene.space.currentZone.getBoneFor(binding.sourceName).position.z;
                    break;

                case SourceType.BoneRotRoll:
                    src = insScene.space.currentZone.getBoneFor(binding.sourceName).rotation.roll.degrees;
                    break;

                case SourceType.BoneRotPitch:
                    src = insScene.space.currentZone.getBoneFor(binding.sourceName).rotation.pitch.degrees;
                    break;

                case SourceType.BoneRotYaw:
                    src = insScene.space.currentZone.getBoneFor(binding.sourceName).rotation.yaw.degrees;
                    break;

                default: assert(0);
            }
        }

        // Smoothly transition back to default pose if tracking is lost.
        if (!insScene.space.hasAnyFocus()) {
            binding.param.value.vector[binding.axis] = dampen(binding.param.value.vector[binding.axis], binding.param.defaults.vector[binding.axis], deltaTime(), 1);
            
            // Fix anoying -e values from dampening
            binding.param.value.vector[binding.axis] = quantize(binding.param.value.vector[binding.axis], 0.0001);
            return;
        }

        // Calculate the input ratio (within 0->1)
        float target = mapValue(src, inRange.x, inRange.y);
        if (inverse) target = 1f-target;

        // NOTE: Dampen level of 0 = no damping
        // Dampen level 1-10 is inverse due to the dampen function taking *speed* as a value.
        if (binding.dampenLevel == 0) binding.inVal = target;
        else {
            binding.inVal = dampen(binding.inVal, target, deltaTime(), cast(float)(11-binding.dampenLevel));
            binding.inVal = quantize(binding.inVal, 0.0001);
        }
        
        // Calculate the output ratio (whatever outRange is)
        binding.outVal = unmapValue(binding.inVal, outRange.x, outRange.y);
        binding.param.value.vector[binding.axis] = binding.outVal;
    }
}

class ExpressionTrackingBinding : ITrackingBinding {
private:
    TrackingBinding binding;
public:
    this(TrackingBinding binding) {
        this.binding = binding;
        expr = new Expression(insExpressionGenerateSignature(cast(int)binding.hashOf(), binding.axis), "");        
    }

    /**
        Expression (if in ExpressionBinding mode)
    */
    Expression* expr;


    override
    void serializeSelf(ref Serializer serializer) {
        serializer.putKey("expression");
        serializer.putValue(expr.expression());
    }
    
    override
    SerdeException deserializeFromFghj(Fghj data) {
        string exprStr;
        data["expression"].deserializeValue(exprStr);
        expr = new Expression(insExpressionGenerateSignature(cast(int)binding.hashOf(), binding.axis), exprStr);
        return null;
    }

    /**
        Sets the parameter out range to the default for the axis
    */
    void outRangeToDefault() {}

    /**
        Updates the parameter binding
    */
    void update() {
        if (binding.sourceName.length == 0) {
            binding.param.value.vector[binding.axis] = binding.param.defaults.vector[binding.axis];
            return;
        }
        if (expr) {

            // Skip NaN values
            float src = expr.call();
            if (!src.isFinite) return;

            // No dampen, or dampen
            if (binding.dampenLevel == 0) binding.outVal = src;
            else {
                
                binding.outVal = dampen(binding.outVal, src, deltaTime(), cast(float)(11-binding.dampenLevel));
                binding.outVal = quantize(binding.outVal, 0.0001);
            }

            binding.param.value.vector[binding.axis] = binding.param.unmapAxis(binding.axis, binding.outVal);
        }
    }
}

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
    TrackingBinding binding;
public:
    this(TrackingBinding binding) {
        this.binding = binding;
        valueMap.length = 0;
    }

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
        serializer.putKey("value_map");
        auto state = serializer.arrayBegin;
            foreach (item; valueMap) {
                serializer.putKey("type");
                serializer.serializeValue(item.type);
                serializer.putKey("id");
                serializer.putValue(item.id);
                serializer.putKey("value");
                serializer.putValue(item.value);
            }
        serializer.arrayEnd(state);
    }
    
    override
    SerdeException deserializeFromFghj(Fghj data) {
        valueMap.length = 0;
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
    void update() {
        if (binding.sourceName.length == 0) {
            binding.param.value.vector[binding.axis] = binding.param.defaults.vector[binding.axis];
            return;
        }
        float src = 0;
        foreach (item; valueMap) {
            if (item.id !in keyMap) continue;
            if (igIsKeyDown(keyMap[item.id.toUpper()])) {
                src = item.value;
                break;
            }
        }
        if (binding.dampenLevel == 0) binding.outVal = src;
        else {
            binding.outVal = dampen(binding.outVal, src, deltaTime(), cast(float)(11-binding.dampenLevel));
            binding.outVal = quantize(binding.outVal, 0.0001);
        }
        binding.param.value.vector[binding.axis] = binding.param.unmapAxis(binding.axis, binding.outVal);
    }
}