#pragma once
#ifdef NIJIEXPOSE_VULKAN
#include "vulkan/backend.hpp"
namespace nijiexpose { using ApplicationRenderBackend=vulkan::Backend; }
#else
#include "nijilive/opengl/backend.hpp"
namespace nijiexpose { using ApplicationRenderBackend=ogl::Backend; }
#endif
