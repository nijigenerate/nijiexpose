"""Translate nijilive's GPU difference collection and tile reduction algorithm."""
from pathlib import Path
import re
import importlib.util
spec=importlib.util.spec_from_file_location('runtime_port',Path(__file__).with_name('port-gl-runtime.py'))
rt=importlib.util.module_from_spec(spec)
spec.loader.exec_module(rt)
code=(rt.source/'diff_collect_impl.d').read_text(encoding='utf-8')
code=rt.common(code[code.index('private struct ReductionLevel'):code.index('\n} else {')])
code=code.replace('this.region','this->region').replace('uint pendingSampleCount','uint32_t pendingSampleCount')
code=code.replace('ReductionLevel[]','std::vector<ReductionLevel>').replace('reductionLevels ~= level;','reductionLevels.push_back(level);').replace('reductionLevels.length = 0;','reductionLevels.clear();')
code=code.replace('foreach (ref level; reductionLevels)','for (auto& level : reductionLevels)')
code=code.replace('out DifferenceEvaluationResult result','DifferenceEvaluationResult& result').replace('DifferenceEvaluationResult.init','DifferenceEvaluationResult{}')
code=code.replace('RenderResourceHandle texture','GLuint texture').replace('~this()', '~DifferenceEvaluator()')
code=code.replace('float[] data = new float[pixelCount * 4];','std::vector<float> data(pixelCount * 4);')
code=code.replace('GLint[4] prevViewport;', 'std::array<GLint,4> prevViewport;').replace('prevViewport.ptr','prevViewport.data()')
code=code.replace('double[TileCount] tileTotals = 0;', 'std::array<double,TileCount> tileTotals{};').replace('double[TileCount] tileCounts = 0;', 'std::array<double,TileCount> tileCounts{};')
code=re.sub(r'foreach \(int (\w+); (.*?) \.\. (.*?)\)',r'for (int \1 = \2; \1 < \3; ++\1)',code)
code=code.replace('foreach (ref count; tileCounts)', 'for (auto& count : tileCounts)')
code=re.sub(r'cast\((uint|size_t|double)\)\((\w+)\)',lambda m:'static_cast<'+('uint32_t' if m[1]=='uint' else m[1])+'>('+m[2]+')',code)
code=re.sub(r'cast\((uint|size_t|double)\)(\w+)',lambda m:'static_cast<'+('uint32_t' if m[1]=='uint' else m[1])+'>('+m[2]+')',code)
code=code.replace('static_cast<int>(floor)(', 'static_cast<int>(floor(') if False else code
# Casts of function expressions need the complete expression, not just its identifier.
code=re.sub(r'static_cast<int>\(floor\)\((.*?)\);',r'static_cast<int>(floor(\1));',code)
code=code.replace('string source','const std::string& source').replace('source.ptr','source.c_str()')
code=code.replace('string message','std::string message').replace('char[] buffer = new char[length];', "std::string buffer(length, '\\0');").replace('buffer.ptr','buffer.data()').replace('cast(string)buffer','buffer')
code=code.replace('throw new Exception(', 'throw std::runtime_error(').replace(' ~ message',' + message')
code=code.replace('enforce(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,\n            "Failed to create framebuffer for difference aggregation");','if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) throw std::runtime_error("Failed to create framebuffer for difference aggregation");')
code=code.replace('int height;\n}', 'int height;\n};',1)
code=code.replace('\n}\n\nDifferenceEvaluator gDifferenceEvaluator;', '\n};\n\nthread_local DifferenceEvaluator gDifferenceEvaluator;')
# D scalar fields start at zero. This aggregate is constructed as a module variable.
for name,path in [('DifferenceVertex','opengl/difference_collect.vert'),('DifferenceCollect','opengl/difference_collect.frag'),('DifferenceReduce','opengl/difference_reduce.frag')]:
    text=(rt.root.parent/'nijilive/shaders'/path).read_text(encoding='utf-8')
    code=code.replace('import("'+path+'")', 'R"NIJIGLSL('+text+')NIJIGLSL"')
prefix='''// Port of nijilive/core/render/backends/opengl/diff_collect_impl.d.
#include "difference.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace nijiexpose::ogl {
using std::clamp; using std::min; using std::max; using std::floor;
constexpr int TileColumns = 16;
constexpr size_t TileCount = TileColumns * TileColumns;
static GLuint compileShader(GLenum,const std::string&);
static GLuint linkProgram(GLuint,GLuint);
'''
(rt.dest/'difference.cpp').write_text(prefix+code+'\n}\n',encoding='utf-8')

