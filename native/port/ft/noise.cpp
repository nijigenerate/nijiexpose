// Ported from inmath 1.0.6 noise/opensimplex.d (MIT).
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
Grad2 grad2[] = {
        Grad2{ 0.130526192220052,  0.99144486137381},
        Grad2{ 0.38268343236509,   0.923879532511287},
        Grad2{ 0.608761429008721,  0.793353340291235},
        Grad2{ 0.793353340291235,  0.608761429008721},
        Grad2{ 0.923879532511287,  0.38268343236509},
        Grad2{ 0.99144486137381,   0.130526192220051},
        Grad2{ 0.99144486137381,  -0.130526192220051},
        Grad2{ 0.923879532511287, -0.38268343236509},
        Grad2{ 0.793353340291235, -0.60876142900872},
        Grad2{ 0.608761429008721, -0.793353340291235},
        Grad2{ 0.38268343236509,  -0.923879532511287},
        Grad2{ 0.130526192220052, -0.99144486137381},
        Grad2{-0.130526192220052, -0.99144486137381},
        Grad2{-0.38268343236509,  -0.923879532511287},
        Grad2{-0.608761429008721, -0.793353340291235},
        Grad2{-0.793353340291235, -0.608761429008721},
        Grad2{-0.923879532511287, -0.38268343236509},
        Grad2{-0.99144486137381,  -0.130526192220052},
        Grad2{-0.99144486137381,   0.130526192220051},
        Grad2{-0.923879532511287,  0.38268343236509},
        Grad2{-0.793353340291235,  0.608761429008721},
        Grad2{-0.608761429008721,  0.793353340291235},
        Grad2{-0.38268343236509,   0.923879532511287},
        Grad2{-0.130526192220052,  0.99144486137381}
    };

    for (int i = 0; i < std::size(grad2); i++) {
        grad2[i].dx /= N2; grad2[i].dy /= N2;
    }
    for (int i = 0; i < PSIZE; i++) {
        GRADIENTS_2D[i] = grad2[i % std::size(grad2)];
    }

    }} init;
double _osnoise2(double xs, double ys) {
        double value = 0;

        // Get base points and offsets
        int xsb = int(std::floor(xs)), ysb = int(std::floor(ys));
        double xsi = xs - xsb, ysi = ys - ysb;

        // Index to point list
        int index = int((ysi - xsi) / 2 + 1);

        double ssi = (xsi + ysi) * -0.211324865405187;
        double xi = xsi + ssi, yi = ysi + ssi;

        // Point contributions
        for (int i = 0; i < 3; i++) {
            const auto& c = LOOKUP_2D[index + i];

            double dx = xi + c.dx, dy = yi + c.dy;
            double attn = 0.5 - dx * dx - dy * dy;
            if (attn <= 0) continue;

            int pxm = (xsb + c.xsv) & PMASK, pym = (ysb + c.ysv) & PMASK;
            Grad2 grad = permGrad2[perm[pxm] ^ pym];
            double extrapolation = grad.dx * dx + grad.dy * dy;

            attn *= attn;
            value += attn * attn * extrapolation;
        }

        return value;
    }
}
void osseed(uint64_t seed) {
    std::array<short,PSIZE> source{};
    for (short i = 0; i < PSIZE; i++)
        source[i] = i;
    for (int i = PSIZE - 1; i >= 0; i--) {
        seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
        int r = int((seed + 31) % (i + 1));
        if (r < 0)
            r += (i + 1);
        perm[i] = source[r];
        permGrad2[i] = GRADIENTS_2D[perm[i]];
        source[r] = source[i];
    }
}
double osnoise2(double x, double y) {

    // Get points for A2* lattice
    double s = 0.366025403784439 * (x + y);
    double xs = x + s, ys = y + s;

    return _osnoise2(xs, ys);
}
}
