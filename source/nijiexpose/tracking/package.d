/*
    Copyright © 2022, Inochi2D Project
    Copyright © 2024, nijigenerate Project
    Distributed under the 2-Clause BSD License, see LICENSE file.
    
    Authors: Luna Nielsen
*/
module nijiexpose.tracking;
import nijiexpose.tracking.expr;
import nijiexpose.tracking.vspace;
public import nijiexpose.tracking.ratiobinding;
public import nijiexpose.tracking.exprbinding;
public import nijiexpose.tracking.eventbinding;
public import nijiexpose.tracking.compoundbinding;
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
        A Binding which combined values of sub-bindings.
    */
    CompoundBinding,

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
    bool update(out float result);
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

    /**
        Display name for the binding
    */
    string name;

    /**
        The type of the binding
    */
    BindingType type_;

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
            case BindingType.CompoundBinding:
                delegated = new CompoundTrackingBinding(this);
                break;
            default:
                break;
        }
        ///
    }

    void serialize(S)(ref S serializer) {
        auto state = serializer.structBegin;
            serializer.putKey("name");
            serializer.putValue(name);
            serializer.putKey("bindingType");
            serializer.serializeValue(type_);
            serializer.putKey("param");
            serializer.serializeValue(param.uuid);
            serializer.putKey("axis");
            serializer.putValue(axis);

            if (delegated)
                delegated.serializeSelf(serializer);

        serializer.structEnd(state);
    }
    
    SerdeException deserializeFromFghj(Fghj data) {
        data["name"].deserializeValue(name);
        data["bindingType"].deserializeValue(type_);
        type = type_;
        data["param"].deserializeValue(paramUUID);
        if (!data["axis"].isEmpty) data["axis"].deserializeValue(axis);

        if (delegated) {
            delegated.deserializeFromFghj(data);
        }
        
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
        if (delegated) {
            float updatedValue;
            if (delegated.update(updatedValue)) {
                param.value.vector[axis] = updatedValue;
            }
        }
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
}
