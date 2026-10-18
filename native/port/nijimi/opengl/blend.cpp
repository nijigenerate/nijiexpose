// Nijimi RenderingBackend blend equations.
// Port of nijilive/core/render/backends/opengl; see project BSD license.
#include "resources.hpp"
#include <array>
#include <limits>
#include <stdexcept>
namespace nijiexpose::ogl {
void oglSetAdvancedBlendCoherent(bool enable) { (void)enable; }

void oglSetLegacyBlendMode(BlendMode blendingMode) {
        switch (blendingMode) {
            case BlendMode::Normal:
                glBlendEquation(GL_FUNC_ADD);
                glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                break;
            case BlendMode::Multiply:
                glBlendEquation(GL_FUNC_ADD);
                glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
                break;
            case BlendMode::Screen:
                glBlendEquation(GL_FUNC_ADD);
                glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);
                break;
            case BlendMode::Lighten:
                glBlendEquation(GL_MAX);
                glBlendFunc(GL_ONE, GL_ONE);
                break;
            case BlendMode::ColorDodge:
                glBlendEquation(GL_FUNC_ADD);
                glBlendFunc(GL_DST_COLOR, GL_ONE);
                break;
            case BlendMode::LinearDodge:
                glBlendEquation(GL_FUNC_ADD);
                glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_COLOR, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                break;
            case BlendMode::AddGlow:
                glBlendEquation(GL_FUNC_ADD);
                glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                break;
            case BlendMode::Subtract:
                glBlendEquationSeparate(GL_FUNC_REVERSE_SUBTRACT, GL_FUNC_ADD);
                glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE);
                break;
            case BlendMode::Exclusion:
                glBlendEquation(GL_FUNC_ADD);
                glBlendFuncSeparate(GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_COLOR, GL_ONE, GL_ONE);
                break;
            case BlendMode::Inverse:
                glBlendEquation(GL_FUNC_ADD);
                glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
                break;
            case BlendMode::DestinationIn:
                glBlendEquation(GL_FUNC_ADD);
                glBlendFunc(GL_ZERO, GL_SRC_ALPHA);
                break;
            case BlendMode::ClipToLower:
                glBlendEquation(GL_FUNC_ADD);
                glBlendFunc(GL_DST_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                break;
            case BlendMode::SliceFromLower:
                glBlendEquation(GL_FUNC_ADD);
                glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
                break;
            default:
                glBlendEquation(GL_FUNC_ADD);
                glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                break;
        }
    }

void oglSetAdvancedBlendEquation(BlendMode blendingMode) {
        // Fallback: use legacy equation path.
        oglSetLegacyBlendMode(blendingMode);
    }


}
