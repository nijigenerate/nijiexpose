module logging_smoke;
import nijiexpose.log;
import std.stdio;
void main() {
    insInitLogging();
    insLogInfo("%s", "logging smoke test: 100%");
    writeln("direct stdout smoke test");
    stderr.writeln("direct stderr smoke test");
    import core.stdc.stdio : printf;
    printf("direct C stdout smoke test\n");
    insLogInfo("invalid %s %s", 1);
}
