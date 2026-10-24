#pragma once
#include "shader.hpp"
#include <core/render/common.hpp>
namespace nijiexpose::ogl {
using MaskApplyPacket = nicxlive::core::RenderBackend::MaskApplyPacket;
using MaskDrawableKind = nicxlive::core::RenderBackend::MaskDrawableKind;
using MaskDrawPacket = nicxlive::core::nodes::MaskDrawPacket;
void oglInitMaskBackend();
void oglBeginMask(bool useStencil);
void oglEndMask();
void oglBeginMaskContent();
void oglExecuteMaskPacket(const MaskDrawPacket& packet);
void oglExecuteMaskApplyPacket(const MaskApplyPacket& packet);
}
