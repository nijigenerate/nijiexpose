"""Translate the current, independently authored nijilive GL resource functions.

Only the rendering branches are copied. Generated files are reviewed C++ sources,
not build-time dependencies on the D checkout.
"""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[2]
source = root.parent / 'nijilive/source/nijilive/core/render/backends/opengl'
dest = root / 'native/port/nijilive/opengl'
dest.mkdir(parents=True, exist_ok=True)
texture = (source / 'texture_backend.d').read_text(encoding='utf-8')
texture = texture[texture.index('private GLuint channelFormat'):texture.index('\n} else {', texture.index('private GLuint channelFormat'))]
texture = texture.replace('private GLuint', 'static GLuint')
texture = texture.replace('ref GLId id', 'GLId& id').replace('uint unit', 'unsigned unit')
texture = texture.replace('ubyte[] data', 'std::span<const uint8_t> data').replace('ubyte[] buffer', 'std::span<uint8_t> buffer')
texture = texture.replace('data.ptr', 'data.data()').replace('buffer.ptr', 'buffer.data()').replace('null', 'nullptr')
texture = texture.replace('Filtering.', 'Filtering::').replace('Wrapping.', 'Wrapping::')
texture = texture.replace('enforce(handle != 0, "Failed to create texture");', 'if (handle == 0) throw std::runtime_error("Failed to create texture");')
texture = texture.replace('[0f, 0f, 0f, 0f].ptr', 'std::array<float, 4>{0.f, 0.f, 0.f, 0.f}.data()')
texture = texture.replace('bool useMipmaps = true', 'bool useMipmaps')
texture = texture.replace('float max;', 'float max = std::numeric_limits<float>::quiet_NaN();')
shader = (source / 'shader_backend.d').read_text(encoding='utf-8')
shader = shader[shader.index('private void checkShader'):shader.index('\n} else {')]
shader = shader.replace('private void', 'static void').replace('ref ShaderProgramHandle handle', 'ShaderProgramHandle& handle')
shader = shader.replace('string vertex, string fragment', 'const std::string& vertex, const std::string& fragment').replace('string name', 'const std::string& name')
shader = shader.replace('char[] log = new char[length];', "std::string log(length, '\\0');")
shader = shader.replace('throw new Exception(cast(string)log);', 'throw std::runtime_error(log);').replace('throw new Exception(', 'throw std::runtime_error(')
shader = shader.replace('log.ptr', 'log.data()').replace('vertex.toStringz', 'vertex.c_str()').replace('fragment.toStringz', 'fragment.c_str()').replace('name.toStringz', 'name.c_str()')
shader = shader.replace('null', 'nullptr').replace('ShaderProgramHandle.init', 'ShaderProgramHandle{}')
shader = shader.replace('value.ptr', '&value.a.a[0][0]')
blend = (source / 'blend.d').read_text(encoding='utf-8')
blend = blend[blend.index('void oglSetAdvancedBlendCoherent'):blend.index('void oglIssueBlendBarrier')]
blend = blend.replace('BlendMode.', 'BlendMode::')
prefix = '// Port of nijilive/core/render/backends/opengl; see project BSD license.\n#include "resources.hpp"\n#include <array>\n#include <limits>\n#include <stdexcept>\nnamespace nijiexpose::ogl {\n'
for name, code in [('texture', texture), ('shader', shader), ('blend', blend)]:
    (dest / f'{name}.cpp').write_text(prefix + code + '\n}\n', encoding='utf-8')
declarations = []
for code in (texture, shader, blend):
    declarations += re.findall(r'^(?:void|int|float) ogl[^\n{]+', code, re.M)
header = '''#pragma once
#include "api.hpp"
#include <core/math/mat4.hpp>
#include <core/nodes/common.hpp>
#include <span>
#include <string>
namespace nijiexpose::ogl {
using GLId = GLuint;
using nicxlive::core::math::Mat4;
using mat4 = Mat4;
using vec2 = nicxlive::core::math::Vec2;
using vec3 = nicxlive::core::math::Vec3;
using vec4 = nicxlive::core::math::Vec4;
using BlendMode = nicxlive::core::nodes::BlendMode;
enum class Filtering { Linear, Nearest };
// nicxlive's public Wrapping currently omits Mirror; retain the D renderer mode.
enum class Wrapping { Clamp, Repeat, Mirror };
struct ShaderProgramHandle { GLuint program{}, vert{}, frag{}; };
'''
header += '\n'.join(x.strip() + ';' for x in declarations) + '\n}\n'
(dest / 'resources.hpp').write_text(header, encoding='utf-8')
extra = (dest / 'drawable.cpp').read_text(encoding='utf-8') if (dest / 'drawable.cpp').exists() else ''
for filename in ('runtime.cpp', 'debug.cpp', 'part.cpp', 'blending.cpp', 'mask.cpp', 'dynamic_composite.cpp', 'draw_texture.cpp', 'difference.cpp'):
    if (dest / filename).exists():
        extra += (dest / filename).read_text(encoding='utf-8')
calls = sorted(set(re.findall(r'\b(gl[A-Z]\w*)\(', texture + shader + blend + extra)) - {'glUploadFloatVecArray'})
api = '''#pragma once
#define GL_GLEXT_PROTOTYPES 1
#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_opengl_glext.h>
namespace nijiexpose::ogl {
void loadApi();
'''
api += '\n'.join(f'extern decltype(&::{x}) {x};' for x in calls) + '\n}\n'
(dest / 'api.hpp').write_text(api, encoding='utf-8')
impl = '#include "api.hpp"\n#include <SDL3/SDL.h>\n#include <stdexcept>\n#include <string>\nnamespace nijiexpose::ogl {\n'
impl += '\n'.join(f'decltype(&::{x}) {x} = nullptr;' for x in calls)
impl += '\nvoid loadApi() {\n'
for x in calls:
    impl += f'    {x} = reinterpret_cast<decltype({x})>(SDL_GL_GetProcAddress("{x}"));\n'
    impl += f'    if (!{x}) throw std::runtime_error("Missing GL entry point: {x}");\n'
impl += '}\n}\n'
(dest / 'api.cpp').write_text(impl, encoding='utf-8')
