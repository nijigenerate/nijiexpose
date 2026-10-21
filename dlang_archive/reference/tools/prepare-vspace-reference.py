"""Extract original D classes, omitting only JSON methods for the core oracle.

The extracted update, list, blendshape and bone methods are unmodified.
JSON integration is exercised separately against the Boost implementation.
"""
from pathlib import Path
import re
root=Path(__file__).resolve().parents[3]
text=(root/'dlang_archive/source/nijiexpose/tracking/vspace.d').read_text(encoding='utf-8')
text=text[text.index('class VirtualSpace {'):]
for pattern in [r'    void serialize\(S\)',r'    SerdeException deserializeFromFghj']:
    while match:=re.search(pattern,text):
        start=match.start(); opening=text.index('{',match.end()); depth=1; end=opening+1
        while depth:
            depth += (text[end]=='{')-(text[end]=='}'); end+=1
        text=text[:start]+text[end:]
header='module reference_vspace_impl;\nimport ft.adaptor, ft.data;\nimport inmath.linalg, inmath.interpolate;\nimport nijiexpose.log;\n'
out=root/'out/reference-vspace';out.mkdir(exist_ok=True)
(out/'reference_vspace_impl.d').write_text(header+text,encoding='utf-8')
