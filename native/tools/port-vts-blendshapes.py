"""Translate the independent facetrack-d tracking adaptor's expressions."""
import argparse,re
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('source',type=Path);a=p.parse_args()
text=a.source.read_text(encoding='utf-8')
start=text.index('if (this.blendshapes.length > 0)'); opening=text.index('{',start);end=opening+1;depth=1
while depth:
    depth+=(text[end]=='{')-(text[end]=='}');end+=1
text=text[start:end]
text=re.sub(r'//[^\n]*','',text).replace('this.blendshapes.length > 0','!blendshapes.empty()').replace('this.blendshapes','blendshapes')
text=re.sub(r'"([^"]+)" in blendshapes',r'blendshapes.contains("\1")',text)
text=re.sub(r'BlendshapeNames\.(\w+)',r'"\1"',text)
# Original destinations are ft-prefixed; all other subscriptions are reads.
text=re.sub(r'blendshapes\["((?!ft)[^"]+)"\]',r'blendshapes.at("\1")',text)
out=Path(__file__).resolve().parents[1]/'port/ft/vts_blendshapes.inc'
out.write_text('// Generated from facetrack-d 0.8.0 vtsproto.d.\n'+text+'\n',encoding='utf-8')
