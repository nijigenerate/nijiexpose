"""Translate the D immediate textured-quad path without changing its GL sequence."""
from pathlib import Path
import importlib.util
spec = importlib.util.spec_from_file_location('runtime_port', Path(__file__).with_name('port-gl-runtime.py'))
rt = importlib.util.module_from_spec(spec)
spec.loader.exec_module(rt)
code = (rt.source / 'draw_texture.d').read_text(encoding='utf-8')
code = rt.common(code[code.index('private __gshared GLuint'):])
code = code.replace('Texture texture, Part part', 'const Texture* texture, const std::shared_ptr<Part>& part')
code = code.replace('Texture texture', 'const Texture* texture')
code = code.replace('rect area', 'Rect area').replace('rect uvs = rect(0, 0, 1, 1)', 'Rect uvs')
code = code.replace('float opacity = 1', 'float opacity').replace('vec3 color = vec3(1, 1, 1)', 'vec3 color').replace('vec3 screenColor = vec3(0, 0, 0)', 'vec3 screenColor')
code = code.replace('Shader s = nullptr, Camera cam = nullptr', 'Shader s, Camera* cam')
code = code.replace('texture.width()', 'texture->width()').replace('texture.height()', 'texture->height()')
code = code.replace('part.immediateModelMatrix()', 'part->immediateModelMatrix()').replace('part.currentRenderSpace()', 'part->currentRenderSpace()').replace('part.getMesh()', 'part->getMesh()')
for name in ['opacity','tint','screenTint']:
    code = code.replace('part.'+name, 'part->'+name)
code = code.replace('incDrawableBindVAO()', 'oglBindDrawableVao()').replace('texture.bind();', 'oglBindTextureHandle(texture->backendId(), 0);')
code = code.replace('enum vertexCount = 4;', 'constexpr size_t vertexCount = 4;')
start = 'float[vertexCount * 2] vertexSoa;'
for vertex, uv in [('-texWidthP, texWidthP, -texWidthP, texWidthP, -texHeightP, -texHeightP, texHeightP, texHeightP','0,1,0,1,0,0,1,1'), ('area.x, area.x+area.w, area.x, area.x+area.w, area.y, area.y, area.y+area.h, area.y+area.h','uvs.x,uvs.w,uvs.x,uvs.w,uvs.y,uvs.y,uvs.h,uvs.h')]:
    a = code.index(start)
    b = code.index('auto laneBytes', a)
    code = code[:a] + 'std::array<float,8> vertexSoa{'+vertex+'};\n    std::array<float,8> uvSoa{'+uv+'};\n    ' + code[b:]
for name in ['vertexSoa','uvSoa']:
    code = code.replace(name+'.length', name+'.size()').replace(name+'.ptr', name+'.data()')
code = code.replace('(cast(ushort[])[\n        0u, 1u, 2u,\n        2u, 1u, 3u\n    ]).ptr', 'std::array<uint16_t,6>{0,1,2,2,1,3}.data()')
code = code.replace('vec2(cast(float)texture->width(), cast(float)texture->height())', 'vec2{float(texture->width()), float(texture->height())}')
code = code.replace('vec2 halfSize = texSize * 0.5f;', 'vec2 halfSize{texSize.x * 0.5f, texSize.y * 0.5f};')
code = code.replace('Rect area = rect(', 'Rect area{').replace('        texSize.y\n    );', '        texSize.y\n    };')
code = code.replace('rect(0, 0, 1, 1)', 'Rect{0, 0, 1, 1}').replace('if (!cam) cam = inGetCamera();', 'if (!cam) cam = &inGetCamera();')
code = code.replace('cam.matrix', 'cam->matrix()').replace('mat4.scaling(1, 1, 1)', 'mat4::scale(vec3{1,1,1})').replace('vec2(0, 0)', 'vec2{0, 0}')
prefix = '''// Port of nijilive/core/render/backends/opengl/draw_texture.d.
#include "draw_texture.hpp"
#include "drawable.hpp"
#include <core/runtime_state.hpp>
#include <core/nodes/part.hpp>
#include <array>
namespace nijiexpose::ogl {
using nicxlive::core::inGetCamera;
extern Shader partShader;
extern GLint gopacity, gMultColor, gScreenColor, mvp, offset;
'''
(rt.dest / 'draw_texture.cpp').write_text(prefix+code+'\n}\n', encoding='utf-8')
