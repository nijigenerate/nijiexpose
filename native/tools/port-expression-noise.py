"""Port only the original 2D noise path invoked by tracking expressions."""
from pathlib import Path
import argparse,re
p=argparse.ArgumentParser();p.add_argument('source',type=Path);a=p.parse_args()
source=a.source.read_text(encoding='utf-8')
def function(signature):
    start=source.index(signature);opening=source.index('{',start);end=opening+1;depth=1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[start:end]
grad=source[source.index('Grad2[] grad2 = ['):source.index('GRADIENTS_3D = new Grad3[PSIZE]')]
grad=grad.replace('Grad2[] grad2 = [','Grad2 grad2[] = {').replace('];','};',1).replace('grad2.length','std::size(grad2)')
grad=re.sub(r'Grad2\(([^)]+)\)',r'Grad2{\1}',grad)
body=function('double _osnoise2(').replace('cast(int)floor(xs)','int(std::floor(xs))').replace('cast(int)floor(ys)','int(std::floor(ys))').replace('cast(int)','int').replace('LatticePoint2D* c = LOOKUP_2D[index + i];','const auto& c = LOOKUP_2D[index + i];')
seed=function('void osseed(').replace('ulong','uint64_t')
seed=re.sub(r'    (perm|permGrad2|permGrad3|permGrad4) = new [^;]+;\n','',seed)
seed=seed.replace('short[] source = new short[PSIZE];','std::array<short,PSIZE> source{};').replace('cast(int)','int').replace('6_364_136_223_846_793_005L','6364136223846793005ULL').replace('1_442_695_040_888_963_407L','1442695040888963407ULL')
seed=re.sub(r'        permGrad[34]\[i\] = [^;]+;\n','',seed)
header='''// Ported from inmath 1.0.6 noise/opensimplex.d (MIT).
#include "noise.hpp"
#include <array>
#include <cmath>
namespace nijiexpose::math {
namespace {
struct Grad2 {double dx,dy;};
struct LatticePoint2D {
    int xsv,ysv;double dx,dy;
    LatticePoint2D(int x,int y):xsv(x),ysv(y) {
        double ssv=(x+y)*-0.211324865405187;dx=-x-ssv;dy=-y-ssv;
    }
};
constexpr int PSIZE=2048,PMASK=2047;
constexpr double N2=0.01001634121365712;
std::array<short,PSIZE> perm{};
std::array<Grad2,PSIZE> permGrad2{},GRADIENTS_2D{};
const std::array<LatticePoint2D,4> LOOKUP_2D={LatticePoint2D(1,0),LatticePoint2D(0,0),LatticePoint2D(1,1),LatticePoint2D(0,1)};
struct Init {Init() {
'''
text=header+grad+'}} init;\n'+body+'\n}\n'+seed+'\n'+function('double osnoise2(')+'\n}\n'
out=Path(__file__).resolve().parents[1]/'port/ft/noise.cpp';out.write_text(text,encoding='utf-8')
