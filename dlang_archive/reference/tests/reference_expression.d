import reference_expression_impl,expression_context,reference_vspace_impl;
import std.stdio,std.string;
import inmath.noise;
void main() {
    insScene.space=new VirtualSpace;
    insInitExpressions();osseed(12345);
    foreach(line;stdin.byLine) {
        auto expr=Expression(insExpressionGenerateSignature(123,0),line.idup.strip);
        float value=expr.call();
        writefln("%s|%s",*cast(uint*)&value,expr.lastError);
    }
    insCleanupExpressions();
}
