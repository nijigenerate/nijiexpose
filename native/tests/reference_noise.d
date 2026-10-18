import inmath.noise;import std.stdio;
void main() {
    foreach(ulong seed;[0UL,1UL,123456789UL,ulong.max]) {
        osseed(seed);
        foreach(i; -100..101) {
            double v=osnoise2(i*0.125,i*0.03);
            writeln(*cast(ulong*)&v);
        }
    }
}
