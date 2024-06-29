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
    TrackingBinding binding_;

public:
    this(TrackingBinding binding) {
        this.binding_ = binding;
        bindingMap.length = 0;
    }

    float outVal = 0;
    final TrackingBinding binding() { return binding_; }

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
        BindingType type_;
        ITrackingBinding delegated;
        CompoundTrackingBinding compound;
        this(CompoundTrackingBinding binding, float weight, BindingType type) {
            this.weight = weight;
            this.type_  = type;
            this.compound = binding;
            this.delegated = binding.createBinding(type);
        }

        BindingType type() { return type_; }
        void type(BindingType value) {
            type_ = value;
            delegated = compound.createBinding(type_);
        }

    }
    BindingMap[] bindingMap;

    ITrackingBinding createBinding(BindingType type) {
        switch (type) {
            case BindingType.RatioBinding:
                return new RatioTrackingBinding(binding);
            case BindingType.ExpressionBinding:
                return new ExpressionTrackingBinding(binding);
            case BindingType.EventBinding:
                return new EventTrackingBinding(binding);
            case BindingType.CompoundBinding:
                return new CompoundTrackingBinding(binding);
            default:
                return null;
        }
    }

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
            elem["type"].deserializeValue(item.type_);
            elem["weight"].deserializeValue(item.weight);
            item.delegated = createBinding(item.type);
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
        outVal = sum;
        result = outVal;
        return true;
    }
}