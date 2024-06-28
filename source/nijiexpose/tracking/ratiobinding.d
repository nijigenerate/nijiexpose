module nijiexpose.tracking.ratiobinding;

import nijiexpose.tracking;
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
    /// Last input value
    float inVal = 0;

    /// Last output value
    float outVal = 0;

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
    bool update(out float result) {
        if (binding.sourceName.length == 0) {
            binding.param.value.vector[binding.axis] = binding.param.defaults.vector[binding.axis];
            return false;
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
            result= dampen(binding.param.value.vector[binding.axis], binding.param.defaults.vector[binding.axis], deltaTime(), 1);
            
            // Fix anoying -e values from dampening
            result = quantize(binding.param.value.vector[binding.axis], 0.0001);
            return true;
        }

        // Calculate the input ratio (within 0->1)
        float target = mapValue(src, inRange.x, inRange.y);
        if (inverse) target = 1f-target;

        // NOTE: Dampen level of 0 = no damping
        // Dampen level 1-10 is inverse due to the dampen function taking *speed* as a value.
        if (binding.dampenLevel == 0) inVal = target;
        else {
            inVal = dampen(inVal, target, deltaTime(), cast(float)(11-binding.dampenLevel));
            inVal = quantize(inVal, 0.0001);
        }
        
        // Calculate the output ratio (whatever outRange is)
        outVal = unmapValue(inVal, outRange.x, outRange.y);
        result = outVal;
        return true;
    }
}