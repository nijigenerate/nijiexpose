#pragma once
#include "resources.hpp"
#include <core/render/common.hpp>
namespace nijiexpose::ogl {
using nicxlive::core::DynamicCompositePass;
using nicxlive::core::DynamicCompositeSurface;
void oglCollectDynamicFramebuffers(bool all=false);
void oglBeginDynamicComposite(DynamicCompositePass& pass);
void oglEndDynamicComposite(DynamicCompositePass& pass);
void oglDestroyDynamicComposite(const std::shared_ptr<DynamicCompositeSurface>& surface);
}
