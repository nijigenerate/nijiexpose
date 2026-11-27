#pragma once
#ifdef NIJIEXPOSE_VULKAN
#include "vulkan/backend.hpp"
namespace nijiexpose { using ApplicationRenderBackend=vulkan::Backend; }
#elif defined(NIJIEXPOSE_METAL)
#include "metal/backend.hpp"
namespace nijiexpose { using ApplicationRenderBackend=metal::Backend; }
#else
#include "nijilive/opengl/backend.hpp"
namespace nijiexpose { using ApplicationRenderBackend=ogl::Backend; }
#endif
#if defined(NIJIEXPOSE_VULKAN) || defined(NIJIEXPOSE_METAL)
namespace nijiexpose {
inline ApplicationRenderBackend& applicationRenderBackend() {
#ifdef NIJIEXPOSE_METAL
    return metal::backend();
#else
    return vulkan::backend();
#endif
}
}
#endif
