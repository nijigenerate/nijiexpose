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
    TrackingBinding binding;

public:
    this(TrackingBinding binding) {
        this.binding = binding;
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
    bool update(out float result) {
        if (expr) {

            // Skip NaN values
            float src = expr.call();
            if (!src.isFinite) return false;

            // No dampen, or dampen
            if (binding.dampenLevel == 0) outVal = src;
            else {
                
                outVal = dampen(outVal, src, deltaTime(), cast(float)(11-binding.dampenLevel));
                outVal = quantize(outVal, 0.0001);
            }

            result = binding.param.unmapAxis(binding.axis, outVal);
            return true;
        }
        return false;
    }
}