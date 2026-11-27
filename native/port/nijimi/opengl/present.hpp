#pragma once
namespace nijiexpose::ogl {
enum class PresentationMode { Opaque = 0, Transparent = 1, ColorKey = 2 };
void oglPresentSceneToBackbuffer(int width, int height, PresentationMode mode);
}
