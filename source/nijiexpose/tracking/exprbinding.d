module nijiexpose.tracking.exprbinding;

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

class ExpressionTrackingBinding : ITrackingBinding {
private:
    TrackingBinding binding_;

public:
    this(TrackingBinding binding) {
        this.binding_ = binding;
        expr = new Expression(insExpressionGenerateSignature(cast(int)binding.hashOf(), binding.axis), "");        
    }

    /// Last input value
    float inVal = 0;

    /// Last output value
    float outVal = 0;

    /**
        Expression (if in ExpressionBinding mode)
    */
    Expression* expr;

    final TrackingBinding binding() { return binding_; }

    /**
        Dampening level
    */
    int dampenLevel = 0;

    override
    void serializeSelf(ref Serializer serializer) {
        serializer.putKey("expression");
        serializer.putValue(expr.expression());
        serializer.putKey("dampenLevel");
        serializer.putValue(dampenLevel);
    }
    
    override
    SerdeException deserializeFromFghj(Fghj data) {
        string exprStr;
        data["expression"].deserializeValue(exprStr);
        if (!data["dampenLevel"].isEmpty) data["dampenLevel"].deserializeValue(dampenLevel);
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
    bool update(out float result) {
        if (expr) {

            // Skip NaN values
            float src = expr.call();
            if (!src.isFinite) return false;

            // No dampen, or dampen
            if (dampenLevel == 0) outVal = src;
            else {
                
                outVal = dampen(outVal, src, deltaTime(), cast(float)(11-dampenLevel));
                outVal = quantize(outVal, 0.0001);
            }

            result = binding.param.unmapAxis(binding.axis, outVal);
            return true;
        }
        return false;
    }
}
