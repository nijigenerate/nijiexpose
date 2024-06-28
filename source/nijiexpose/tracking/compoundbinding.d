module nijiexpose.tracking.compoundbinding;

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


class CompoundTrackingBinding : ITrackingBinding {
private:
    TrackingBinding binding;
    float outVal;

public:
    this(TrackingBinding binding) {
        this.binding = binding;
        bindingMap.length = 0;
    }


    enum Method {
        WeightedSum,
        WeightedMul,
        Ordered,
    }
    Method method = Method.WeightedSum;
    /**
        Expression (if in ExpressionBinding mode)
    */
    struct BindingMap {
        float weight = 1.0;
        BindingType type;
        ITrackingBinding delegated;
    }
    BindingMap[] bindingMap;

    override
    void serializeSelf(ref Serializer serializer) {
        serializer.putKey("binding_map");
        auto state = serializer.arrayBegin;
            foreach (item; bindingMap) {
                serializer.elemBegin;
                auto state2 = serializer.objectBegin();
                serializer.putKey("type");
                serializer.serializeValue(item.type);
                serializer.putKey("weight");
                serializer.serializeValue(item.weight);
                item.delegated.serializeSelf(serializer);
                serializer.objectEnd(state2);
            }
        serializer.arrayEnd(state);
    }
    
    override
    SerdeException deserializeFromFghj(Fghj data) {
        bindingMap.length = 0;
        foreach (elem; data["binding_map"].byElement) {
            BindingMap item;
            elem["type"].deserializeValue(item.type);
            elem["weight"].deserializeValue(item.weight);
            switch (item.type) {
                case BindingType.RatioBinding:
                    item.delegated = new RatioTrackingBinding(binding);
                    break;
                case BindingType.ExpressionBinding:
                    item.delegated = new ExpressionTrackingBinding(binding);
                    break;
                case BindingType.EventBinding:
                    item.delegated = new EventTrackingBinding(binding);
                    break;
                case BindingType.CompoundBinding:
                    item.delegated = new CompoundTrackingBinding(binding);
                    break;
                default:
                    continue;
            }
            item.delegated.deserializeFromFghj(elem);
            bindingMap ~= item;
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
        float sum = 0;
        float weightSum = 0;
        foreach (item; bindingMap) {
            float src;
            if (!item.delegated.update(src)) continue;
            switch (method) {
                case Method.WeightedSum:
                    weightSum += item.weight;
                    sum += item.weight * src;
                    break;
                case Method.WeightedMul:
                    if (item.weight == 0) continue;
                    sum *= item.weight * src;
                    break;
                case Method.Ordered:
                    if (item.weight > weightSum) {
                        sum = src;
                        weightSum = item.weight;
                    }
                    break;
                default:
                    break;
            }
        }
        if (method == Method.WeightedSum)
            if (weightSum > 0)
                sum /= weightSum;
        if (binding.dampenLevel == 0) outVal = sum;
        else {
            outVal = dampen(outVal, sum, deltaTime(), cast(float)(11-binding.dampenLevel));
            outVal = quantize(outVal, 0.0001);
        }
        result = binding.param.unmapAxis(binding.axis, outVal);
        return true;
    }
}