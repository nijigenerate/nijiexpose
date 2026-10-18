"""Translate the rendering branch of nijilive's mask backend."""
from pathlib import Path
import importlib.util
spec = importlib.util.spec_from_file_location('runtime_port', Path(__file__).with_name('port-gl-runtime.py'))
rt = importlib.util.module_from_spec(spec)
spec.loader.exec_module(rt)
code = (rt.source / 'mask.d').read_text(encoding='utf-8')
code = code[code.index('private __gshared Shader'):code.index('\n} else {')]
code = rt.common(code).replace('ref MaskDrawPacket packet', 'const MaskDrawPacket& packet').replace('ref MaskApplyPacket packet', 'const MaskApplyPacket& packet')
code = code.replace('incDrawableBindVAO()', 'oglBindDrawableVao()').replace('final switch', 'switch').replace('MaskDrawableKind.', 'MaskDrawableKind::')
# nicxlive has an extra Drawable tag; preserve D's closed enum requirement.
code = code.replace('case MaskDrawableKind::Mask:', 'case MaskDrawableKind::Drawable:\n            throw std::runtime_error("Unexpected Drawable mask packet; D packet kind must be Part or Mask");\n        case MaskDrawableKind::Mask:')
prefix = '''// Port of nijilive/core/render/backends/opengl/mask.d.
#include "mask.hpp"
#include "part.hpp"
#include "drawable.hpp"
#include "mask_assets.hpp"
#include <stdexcept>
namespace nijiexpose::ogl {
// D's buffer_sync implementation intentionally disables fences.
static void markBufferInUse(GLuint) {}
'''
(rt.dest / 'mask.cpp').write_text(prefix + code + '\n}\n', encoding='utf-8')
paths = rt.assets['MaskShaderSource']
texts = [(rt.root.parent / 'nijilive/shaders' / p).read_text(encoding='utf-8') for p in paths]
(rt.dest / 'mask_assets.hpp').write_text('#pragma once\n#include "shader.hpp"\nnamespace nijiexpose::ogl {\n' + f'inline const ShaderAsset MaskShaderSource{{R"NIJIGLSL({texts[0]})NIJIGLSL", R"NIJIGLSL({texts[1]})NIJIGLSL"}};\n}}\n', encoding='utf-8')
