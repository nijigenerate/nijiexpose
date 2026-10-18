"""Compile original binding algorithms with fixture scene/parameter/UI edges.

Only serialization/deserialization methods and imports are omitted. Their JSON
integration is checked separately in the native host.
"""
from pathlib import Path
import re
root=Path(__file__).resolve().parents[2]
parts=[]
for name in ['package','ratiobinding','exprbinding','eventbinding','compoundbinding']:
    text=(root/f'source/nijiexpose/tracking/{name}.d').read_text(encoding='utf-8')
    text=re.sub(r'^(?:public )?import [^;]+;|^module [^;]+;','',text,flags=re.M)
    text=re.sub(r'^alias Serializer = [^;]+;','',text,flags=re.M)
    for pattern in [r'(?:    override\s*)?    void serializeSelf\(',r'    void serialize\(S\)',r'(?:    override\s*)?    SerdeException deserializeFromFghj\(']:
        while match:=re.search(pattern,text):
            opening=text.find('{',match.end());semicolon=text.find(';',match.end())
            if semicolon!=-1 and semicolon<opening:
                text=text[:match.start()]+text[semicolon+1:];continue
            end=opening+1;depth=1
            while depth:depth+=(text[end]=='{')-(text[end]=='}');end+=1
            text=text[:match.start()]+text[end:]
    parts.append(text)
keys=','.join(chr(c) for c in range(65,91))
prefix='''module reference_binding_impl;
import inmath, std.format, std.math.rounding, std.math, std.array, std.uni;
import reference_expression_impl, reference_vspace_impl, expression_context;
class Parameter {
 uint uuid;vec2 value=vec2(0),defaults=vec2(0),min=vec2(0),max=vec2(1);
 float unmapAxis(uint axis,float offset) {vec2 input=min;if(axis==0)input.x=offset;else input.y=offset;vec2 range=max-min;vec2 output=vec2(range.x*input.x,range.y*input.y)+min;return axis==0?output.x:output.y;}
}
class Puppet {Parameter findParameter(uint uuid){return null;}}
double deltaTime(){return 0.016;}
float dampen(float pos,float target,double delta,double speed=1){return (pos-target)*pow(0.001,delta*speed)+target;}
enum ImGuiKey {KEYS}
bool keyDown;
bool igIsKeyDown(ImGuiKey key){return key==ImGuiKey.A && keyDown;}
bool neIsEventOn(int key){return false;}
'''.replace('KEYS',keys)
out=root/'out/reference-vspace/reference_binding_impl.d';out.write_text(prefix+'\n'.join(parts),encoding='utf-8')
