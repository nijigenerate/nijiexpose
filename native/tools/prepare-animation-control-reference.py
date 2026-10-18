from pathlib import Path
import re
root=Path(__file__).resolve().parents[2]
text=(root/'source/nijiexpose/animation/package.d').read_text(encoding='utf-8')
text=text.replace('module nijiexpose.animation;','module reference_control_impl;').replace('public import nijiexpose.tracking;','import reference_binding_impl : SourceType;').replace('import nijiexpose.scene;','import expression_context;').replace('import i18n;','import std.string : toStringz;\nconst(char)* __(string s){return s.toStringz;}\ndouble deltaTime(){return 0.016;}')
for pattern in [r'    void serialize\(S\)',r'    SerdeException deserializeFromFghj\(']:
    while match:=re.search(pattern,text):
        opening=text.index('{',match.end());end=opening+1;depth=1
        while depth:depth+=(text[end]=='{')-(text[end]=='}');end+=1
        text=text[:match.start()]+text[end:]
(root/'out/reference-vspace/reference_control_impl.d').write_text(text,encoding='utf-8')
