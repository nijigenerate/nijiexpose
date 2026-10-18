"""Translate all branches of nijilive's current part/blending implementations."""
from pathlib import Path
import re
import importlib.util
spec = importlib.util.spec_from_file_location('runtime_port', Path(__file__).with_name('port-gl-runtime.py'))
rt = importlib.util.module_from_spec(spec)
spec.loader.exec_module(rt)
root, source, dest = rt.root, rt.source, rt.dest
code = (source / 'part.d').read_text(encoding='utf-8')
code = code[code.index('package(nijilive) {'):code.index('\n} else {')]
code = rt.common(code)
code = code.replace(' {\n    __gshared Texture boundAlbedo;', '')
code = code.replace('__gshared ', '')
code = code.replace('\n}\n\nvoid oglInitPartBackendResources', '\n\nvoid oglInitPartBackendResources', 1)
code = code.replace('ref PartDrawPacket packet', 'const PartDrawPacket& packet')
code = code.replace('packet.blendingMode', 'packet.blendMode')
code = code.replace('auto textures = packet.textures;', 'auto node = packet.node.lock();\n    if (!node) return;\n    auto textures = node->textures;')
code = code.replace('textures.length', 'textures.size()')
code = code.replace('incDrawableBindVAO()', 'oglBindDrawableVao()')
code = code.replace('foreach(i, ref texture; textures)', 'for (size_t i = 0; i < textures.size(); ++i)')
code = code.replace('if (texture) texture.bind(cast(uint)i);', 'auto& texture = textures[i];\n            if (texture) oglBindTextureHandle(texture->backendId(), static_cast<unsigned>(i));')
code = code.replace('cast(uint)i', 'static_cast<unsigned>(i)')
code = code.replace('foreach(att; 0 .. 3)', 'for (int att = 0; att < 3; ++att)')
code = code.replace('boundAlbedo = nullptr;', 'boundAlbedo.reset();')
code = re.sub(r'(GLint|GLfloat|GLboolean)\[4\] (\w+);', r'std::array<\1, 4> \2;', code)
code = code.replace('GLboolean stencilEnabled;\n                    }', 'GLboolean stencilEnabled;\n                    };')
code = code.replace('cast(const(GLenum)*)prev.drawBuffers.ptr', 'reinterpret_cast<const GLenum*>(prev.drawBuffers.data())')
code = code.replace('static_cast<int>(prev.drawBuffers.length)', 'static_cast<int>(prev.drawBuffers.size())')
code = re.sub(r'prev\.(\w+)\.ptr', r'prev.\1.data()', code)
prefix = '''// Port of nijilive/core/render/backends/opengl/part.d, all rendering branches.
#include "part.hpp"
#include "blending.hpp"
#include "runtime.hpp"
#include "drawable.hpp"
#include "part_assets.hpp"
#include <core/nodes/part.hpp>
#include <core/runtime_state.hpp>
#include <algorithm>
namespace nijiexpose::ogl {
using std::min;
using nicxlive::core::inGetViewport;
static void setupShaderStage(const PartDrawPacket&, int, mat4, mat4);
static void renderStage(const PartDrawPacket&, bool);
// The D buffer_sync module deliberately disables fences and does no work here.
static void markBufferInUse(GLuint) {}
'''
(dest / 'part.cpp').write_text(prefix + code + '\n}\n', encoding='utf-8')
blend = (source / 'blend.d').read_text(encoding='utf-8')
blend = blend[blend.index('private __gshared Shader'):blend.index('void oglSetAdvancedBlendCoherent')]
blend = rt.common(blend).replace('Shader[BlendMode]', 'std::unordered_map<BlendMode, Shader>')
blend = blend.replace('blendShaders.length', 'blendShaders.size()').replace('BlendMode[] advancedModes = [', 'std::array advancedModes{').replace('\n    ];', '\n    };')
blend = blend.replace('BlendMode.', 'BlendMode::').replace('foreach (mode; advancedModes)', 'for (auto mode : advancedModes)')
blend = blend.replace('auto shader = mode in blendShaders;\n    return shader ? *shader : nullptr;', 'auto shader = blendShaders.find(mode);\n    return shader != blendShaders.end() ? shader->second : Shader{};')
blend = blend.replace('if (shader == nullptr)', 'if (!shader)')
blend = blend.replace('cast(int)mode', 'static_cast<int>(mode)').replace('mat4.identity', 'mat4::identity()').replace('vec2(0, 0)', 'vec2{0, 0}')
blend = blend.replace('Shader(shaderAsset!("opengl/basic/basic.vert","opengl/basic/advanced_blend.frag")())', 'Shader(AdvancedBlendShaderSource)')
rt.assets['AdvancedBlendShaderSource'] = ('opengl/basic/basic.vert','opengl/basic/advanced_blend.frag')
common = (root.parent / 'nijilive/source/nijilive/core/nodes/common.d').read_text(encoding='utf-8')
state = common[common.index('private {'):common.index('/*\n    INFORMATION ABOUT BLENDING MODES')]
state = state.replace('private {', 'namespace {')
state = re.sub(r'    version\(OSX\).*?bool inForceTripleBufferFallback', '    constexpr bool inDefaultTripleBufferFallback = false;\n    bool inForceTripleBufferFallback', state, flags=re.S)
state = state.replace('BlendMode.', 'BlendMode::')
state = re.sub(r'^(    )bool (in\w+)',r'\1thread_local bool \2',state,flags=re.M)
state = re.sub(r'(case BlendMode::\w+),', r'\1:', state)
state = re.sub(r'\n             (BlendMode::\w+),', r'\n        case \1:', state)
state = state.replace('             BlendMode::SliceFromLower:', '        case BlendMode::SliceFromLower:')
tail = common[common.index('bool inIsAdvancedBlendMode'):common.index('/**\n    Masking mode')]
tail = tail.replace('BlendMode.', 'BlendMode::').replace('bool legacyOnly=false', 'bool legacyOnly')
prefix = '''// Port of nijilive OpenGL blend.d and nodes/common.d blending state.
#include "blending.hpp"
#include "part_assets.hpp"
#include <SDL3/SDL.h>
#include <unordered_map>
#include <array>
namespace nijiexpose::ogl {
static bool hasAdvancedBlendSupport() { return SDL_GL_ExtensionSupported("GL_KHR_blend_equation_advanced"); }
static bool hasAdvancedBlendCoherentSupport() { return SDL_GL_ExtensionSupported("GL_KHR_blend_equation_advanced_coherent"); }
static void setAdvancedBlendCoherent(bool e) { oglSetAdvancedBlendCoherent(e); }
static void setLegacyBlendMode(BlendMode m) { oglSetLegacyBlendMode(m); }
static void setAdvancedBlendEquation(BlendMode m) { oglSetAdvancedBlendEquation(m); }
static void issueBlendBarrier() {
    auto barrier = reinterpret_cast<PFNGLBLENDBARRIERKHRPROC>(SDL_GL_GetProcAddress("glBlendBarrierKHR"));
    if (!barrier) throw std::runtime_error("Missing glBlendBarrierKHR");
    barrier();
}
'''
(dest / 'blending.cpp').write_text(prefix + state + tail + blend + '\n}\n', encoding='utf-8')
assets_code = '#pragma once\n#include "shader.hpp"\nnamespace nijiexpose::ogl {\n'
for name, paths in rt.assets.items():
    if not name.startswith('Part') and name != 'AdvancedBlendShaderSource': continue
    texts = [(root.parent / 'nijilive/shaders' / p).read_text(encoding='utf-8') for p in paths]
    assets_code += f'inline const ShaderAsset {name}{{R"NIJIGLSL({texts[0]})NIJIGLSL", R"NIJIGLSL({texts[1]})NIJIGLSL"}};\n'
(dest / 'part_assets.hpp').write_text(assets_code + '}\n', encoding='utf-8')
