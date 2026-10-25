#pragma once
#include "nijilive/opengl/api.hpp"

#ifdef NIJIEXPOSE_CAPTURE_PRESENTED_FRAME
// Linux test window surfaces need not expose a readable GL_FRONT after swap.
// The Linux interaction test records the exact framebuffer submitted to swap.
inline GLenum presentedReadBuffer() { return GL_BACK; }
void readPresentedPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                         GLenum format, GLenum type, void* output);
#else
inline GLenum presentedReadBuffer() { return GL_FRONT; }
inline void readPresentedPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                                GLenum format, GLenum type, void* output) {
    ::glReadPixels(x, y, width, height, format, type, output);
}
#endif
