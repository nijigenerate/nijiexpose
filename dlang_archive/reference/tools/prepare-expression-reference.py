from pathlib import Path
import re
root=Path(__file__).resolve().parents[3]
text=(root/'dlang_archive/source/nijiexpose/tracking/expr.d').read_text(encoding='utf-8')
text=text.replace('module nijiexpose.tracking.expr;','module reference_expression_impl;\nimport expression_context;')
for module in ['nijiexpose.tracking','nijiexpose.scene','ft','nijilive','i18n']:
    text=text.replace('import '+module+';','')
out=root/'out/reference-vspace';out.mkdir(exist_ok=True)
(out/'reference_expression_impl.d').write_text(text,encoding='utf-8')
(out/'expression_context.d').write_text('''module expression_context;
import reference_vspace_impl;
struct Scene {VirtualSpace space;}
Scene insScene;
double currentTime(){return 1.25;}
string _(string value){return value;}
''',encoding='utf-8')
