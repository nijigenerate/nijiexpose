"""Mechanical C++ translation of current nijilive GL scene/debug sources."""
from pathlib import Path
import re
root = Path(__file__).resolve().parents[2]
source = root.parent / 'nijilive/source/nijilive/core/render/backends/opengl'
dest = root / 'native/port/nijilive/opengl'
assets = {}
def asset(m):
    name, vertex, fragment = m.groups()
    assets[name] = (vertex, fragment)
    return ''
def common(code):
    code = re.sub(r'enum ShaderAsset (\w+) = shaderAsset!\("([^"]+)","([^"]+)"\)\(\);', asset, code)
    code = code.replace('package(nijilive)', '').replace('version(InDoesRender)', '').replace('version (InDoesRender)', '')
    code = code.replace('private {', '{').replace('private:', '').replace('public:', '')
    code = code.replace('private __gshared ', '').replace('private ', '')
    code = code.replace('float.sizeof', 'sizeof(float)').replace('ushort.sizeof', 'sizeof(uint16_t)')
    code = code.replace('null', 'nullptr').replace('new Shader(', 'Shader(')
    code = code.replace(' is nullptr', ' == nullptr').replace(' !is nullptr', ' != nullptr')
    code = re.sub(r'cast\((int|GLuint|ptrdiff_t)\)(\w+(?:\.\w+)*)', r'static_cast<\1>(\2)', code)
    code = re.sub(r'cast\((void|float)\*\)\(([^\n;]+?)\)', r'reinterpret_cast<\1*>(\2)', code)
    code = re.sub(r'cast\((void|float)\*\)(\w+)', r'reinterpret_cast<\1*>(\2)', code)
    code = re.sub(r'\[([A-Z0-9_, ]+)\]\.ptr', lambda m: 'std::array<GLenum, '+str(len(m[1].split(',')))+'>{'+m[1]+'}.data()', code)
    code = code.replace('data.ptr', 'data.data()')
    code = code.replace('float[] data = [', 'std::array<float, 24> data = {').replace('\n    ];', '\n    };')
    return code
code = (source / 'runtime.d').read_text(encoding='utf-8')
code = code[code.index('struct PostProcessingShader'):]
code = common(code)
code = code.replace('GLint[string] uniformCache;', 'std::unordered_map<std::string, GLint> uniformCache;')
code = code.replace('this(Shader shader)', 'PostProcessingShader(Shader shader)').replace('this.shader = shader;', 'this->shader = shader;')
code = code.replace('this.hasUniform', 'this->hasUniform').replace('string name', 'const std::string& name')
code = code.replace('(name in uniformCache) != nullptr', 'uniformCache.contains(name)')
code = code.replace('struct PostProcessingShader {', 'struct PostProcessingShader {\n    PostProcessingShader() = default;')
code = code.replace('PostProcessingShader[]', 'std::vector<PostProcessingShader>')
code = code.replace('ref std::vector<PostProcessingShader>', 'std::vector<PostProcessingShader>&')
code = code.replace('CompositeFrameState[]', 'std::vector<CompositeFrameState>')
code = code.replace('GLint[4] viewport;', 'std::array<GLint, 4> viewport;')
code = code.replace('std::array<GLint, 4> viewport;\n    }', 'std::array<GLint, 4> viewport;\n    };')
code = code.replace('inViewportWidth[$-1]', 'viewportWidth()').replace('inViewportHeight[$-1]', 'viewportHeight()')
code = code.replace('inClearColor.r, inClearColor.g, inClearColor.b, inClearColor.a', 'clearColor().x, clearColor().y, clearColor().z, clearColor().w')
code = code.replace('postProcessingStack.length', 'postProcessingStack.size()')
code = code.replace('bool targetBuffer;', 'bool targetBuffer = false;')
code = code.replace('foreach(shader; postProcessingStack)', 'for (auto shader : postProcessingStack)')
code = code.replace('postProcessingStack ~= PostProcessingShader(Shader(LightingShaderSource));', 'postProcessingStack.push_back(PostProcessingShader(Shader(LightingShaderSource)));')
code = code.replace('ref ubyte[] dumpTo', 'std::vector<uint8_t>& dumpTo').replace('dumpTo.ptr', 'dumpTo.data()')
code = code.replace('inInitDebug();', 'oglInitDebugRenderer();')
code = code.replace('boundAlbedo = nullptr;', 'boundAlbedo.reset();')
code = code.replace('mat4.orthographic', 'mat4::orthographic').replace('mat4.translation', 'mat4::translation')
# D aggregate construction accepts numeric conversions; C++ brace construction is explicit.
code = code.replace('vec2(viewportWidth(), viewportHeight())', 'vec2{float(viewportWidth()), float(viewportHeight())}')
code = code.replace('vec4 area = vec4(\n        0, 0,\n        viewportWidth(), viewportHeight()\n    );', 'vec4 area{0, 0, float(viewportWidth()), float(viewportHeight())};')
# D module visibility blocks become namespaces; the class closing delimiter needs C++ semicolon.
code = code.replace('\n}\n\n// Internal rendering constants', '\n};\n\n// Internal rendering constants')
code = code.replace('// Internal rendering constants\n{', '// Internal rendering constants\nnamespace {')
code = code.replace('// Things only available internally for nijilive rendering\n {', '// Things only available internally for nijilive rendering\nnamespace {')
# oglInitRenderer must retain external linkage; remove its former package block.
code = code.replace('// Things only available internally for nijilive rendering\nnamespace {', '// Things only available internally for nijilive rendering\n')
code = code.replace('\n}\n\n/**\n    Begins rendering', '\n\n/**\n    Begins rendering', 1)
prefix = '''// Port of nijilive/core/render/backends/opengl/runtime.d.
#include "runtime.hpp"
#include "debug.hpp"
#include "assets.hpp"
#include <core/runtime_state.hpp>
#include <array>
#include <algorithm>
#include <unordered_map>
namespace nijiexpose::ogl {
using nicxlive::core::inSetViewport;
using nicxlive::core::inGetClearColor;
using std::max;
thread_local vec3 inSceneAmbientLight{1, 1, 1};
std::shared_ptr<nicxlive::core::Texture> boundAlbedo;
static int viewportWidth() { int w,h; nicxlive::core::inGetViewport(w,h); return w; }
static int viewportHeight() { int w,h; nicxlive::core::inGetViewport(w,h); return h; }
static vec4 clearColor() { vec4 c; inGetClearColor(c.x,c.y,c.z,c.w); return c; }
'''
for name in ('sceneVAO','sceneVBO','fBuffer','fAlbedo','fEmissive','fBump','fStencil','cfBuffer','cfAlbedo','cfEmissive','cfBump','cfStencil','blendFBO','blendAlbedo','blendEmissive','blendBump','blendStencil','basicSceneShader','basicSceneLighting','postProcessingStack','isCompositing','compositeScopeStack'):
    code=re.sub(r'^(    )((?:GLuint|bool|PostProcessingShader|std::vector<\w+>) '+name+r');',r'\1thread_local \2;',code,flags=re.M)
(dest / 'runtime.cpp').write_text(prefix + code + '\n}\n', encoding='utf-8')
debug = (source / 'debug_renderer.d').read_text(encoding='utf-8')
debug = debug[debug.index('private Shader lineShader'):debug.index('\n} else {')]
debug = common(debug)
debug = debug.replace('lineShader != nullptr', 'bool(lineShader)').replace('shader == nullptr', '!shader')
debug = debug.replace('Vec3Array points, ushort[] indices', 'const std::vector<vec3>& points, std::span<const uint16_t> indices')
debug = debug.replace('points.length', 'points.size()').replace('indices.length', 'indices.size()').replace('indices.ptr', 'indices.data()')
debug = debug.replace('RenderResourceHandle', 'GLuint')
debug = debug.replace('glUploadFloatVecArray(vbo, points, GL_DYNAMIC_DRAW, "UploadDebug");', '''std::vector<float> raw(points.size() * 3);
    for (size_t i = 0; i < points.size(); ++i) {
        raw[i] = points[i].x;
        raw[points.size() + i] = points[i].y;
        raw[points.size() * 2 + i] = points[i].z;
    }
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, raw.size() * sizeof(float), raw.data(), GL_DYNAMIC_DRAW);''')
for name in ('lineShader','pointShader','vao','vbo','ibo','currentVbo','indexCount','lineMvpLocation','lineColorLocation','pointMvpLocation','pointColorLocation'):
    debug=re.sub(r'^((?:Shader|GLuint|int) '+name+r'(?: = -1)?);',r'thread_local \1;',debug,flags=re.M)
(dest / 'debug.cpp').write_text('// Port of nijilive/core/render/backends/opengl/debug_renderer.d.\n#include "debug.hpp"\n#include "assets.hpp"\nnamespace nijiexpose::ogl {\n' + debug + '\n}\n', encoding='utf-8')
assets_code = '#pragma once\n#include "shader.hpp"\nnamespace nijiexpose::ogl {\n'
for name, paths in assets.items():
    texts = [(root.parent / 'nijilive/shaders' / p).read_text(encoding='utf-8') for p in paths]
    assets_code += f'inline const ShaderAsset {name}{{R"NIJIGLSL({texts[0]})NIJIGLSL", R"NIJIGLSL({texts[1]})NIJIGLSL"}};\n'
(dest / 'assets.hpp').write_text(assets_code + '}\n', encoding='utf-8')
