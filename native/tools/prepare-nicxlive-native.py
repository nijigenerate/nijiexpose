"""Preserve the application Welding exclusion only.

All other native adaptations live in nicxlive itself.
"""
from pathlib import Path
import argparse
p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
a.output.mkdir(parents=True, exist_ok=True)
def write(name, code):
    path=a.output/name
    path.parent.mkdir(parents=True, exist_ok=True)
    if not path.exists() or path.read_text(encoding='utf-8') != code:
        path.write_text(code,encoding='utf-8')
def replace(code, before, after):
    if code.count(before) != 1:
        raise RuntimeError('Pinned nicxlive source changed: ' + before)
    return code.replace(before, after)
def replace_function(code, signature, implementation):
    start=code.index(signature)
    opening=code.index('{', start)
    end=opening+1
    depth=1
    while depth:
        depth+=(code[end]=='{')-(code[end]=='}')
        end+=1
    return code[:start]+implementation+code[end:]
code = (a.source / 'core/nodes/drawable.cpp').read_text(encoding='utf-8')
code=replace_function(code, 'void Drawable::finalizeDrawable()', '''void Drawable::finalizeDrawable() {
    for(auto& child:children) if(child && child->pinToMesh) setupChild(child);
    std::vector<WeldingLink> valid;
    if(auto pup=puppetRef()) {
for(auto link:weldedLinks) {
    auto target=std::dynamic_pointer_cast<Drawable>(pup->findNodeById(link.targetUUID));
    if(target) {link.target=target;valid.push_back(std::move(link));}
}
    }
    weldedLinks=std::move(valid);
}''')
write('drawable.cpp', code)
