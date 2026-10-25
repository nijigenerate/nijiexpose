#include "presented_frame.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <stdexcept>
#include <vector>

namespace {
std::vector<unsigned char> presented;
int frameWidth = 0, frameHeight = 0;
}

extern "C" bool __real_SDL_GL_SwapWindow(SDL_Window* window);
extern "C" bool __wrap_SDL_GL_SwapWindow(SDL_Window* window) {
    using namespace nijiexpose;
    GLint framebuffer, buffer, alignment, rowLength, skipRows, skipPixels, packBuffer;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &framebuffer);
    glGetIntegerv(GL_READ_BUFFER, &buffer);
    glGetIntegerv(GL_PACK_ALIGNMENT, &alignment);
    glGetIntegerv(GL_PACK_ROW_LENGTH, &rowLength);
    glGetIntegerv(GL_PACK_SKIP_ROWS, &skipRows);
    glGetIntegerv(GL_PACK_SKIP_PIXELS, &skipPixels);
    glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &packBuffer);
    SDL_GetWindowSizeInPixels(window, &frameWidth, &frameHeight);
    presented.resize(static_cast<size_t>(frameWidth) * frameHeight * 4);
    ogl::glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    ogl::glReadBuffer(GL_BACK);
    ogl::glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    ogl::glPixelStorei(GL_PACK_ALIGNMENT, 1);
    ogl::glPixelStorei(GL_PACK_ROW_LENGTH, 0);
    ogl::glPixelStorei(GL_PACK_SKIP_ROWS, 0);
    ogl::glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
    ::glReadPixels(0, 0, frameWidth, frameHeight, GL_RGBA, GL_UNSIGNED_BYTE, presented.data());
    ogl::glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
    ogl::glReadBuffer(buffer);
    ogl::glBindBuffer(GL_PIXEL_PACK_BUFFER, packBuffer);
    ogl::glPixelStorei(GL_PACK_ALIGNMENT, alignment);
    ogl::glPixelStorei(GL_PACK_ROW_LENGTH, rowLength);
    ogl::glPixelStorei(GL_PACK_SKIP_ROWS, skipRows);
    ogl::glPixelStorei(GL_PACK_SKIP_PIXELS, skipPixels);
    return __real_SDL_GL_SwapWindow(window);
}

void readPresentedPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                         GLenum format, GLenum type, void* output) {
    if (type != GL_UNSIGNED_BYTE || (format != GL_RGB && format != GL_RGBA) ||
        x < 0 || y < 0 || width < 0 || height < 0 || x + width > frameWidth || y + height > frameHeight)
        throw std::runtime_error("Invalid presented-frame readback");
    auto* bytes = static_cast<unsigned char*>(output);
    const int channels = format == GL_RGB ? 3 : 4;
    for (int row = 0; row < height; ++row)
        for (int col = 0; col < width; ++col) {
            const auto offset = (static_cast<size_t>(y + row) * frameWidth + x + col) * 4;
            bytes = std::copy_n(presented.data() + offset, channels, bytes);
        }
}
