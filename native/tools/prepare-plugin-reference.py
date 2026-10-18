"""Original plugin and base API with logging/translation edges isolated."""
from pathlib import Path
import re
root=Path(__file__).resolve().parents[2]
out=root/'out/reference-plugin'
out.mkdir(exist_ok=True)
for name,source in [('reference_plugin_impl','plugin.d'),('reference_plugin_base','api/base.d')]:
    text=(root/'source/nijiexpose/plugins'/source).read_text(encoding='utf-8')
    text=re.sub(r'^module [^;]+;|^import nijiexpose[^;]+;|^import i18n;','',text,flags=re.M)
    prefix=f'module {name};\nimport lumars, bindbc.lua;\nstring _(string s){{return s;}}\nvoid insLogInfo(T...)(string fmt,T args){{}}\nvoid insLogWarn(T...)(string fmt,T args){{}}\nvoid insLogErr(T...)(string fmt,T args){{}}\n'
    (out/f'{name}.d').write_text(prefix+text,encoding='utf-8')
