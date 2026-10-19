"""Generate focused native backend adaptations of pinned nicxlive translation units.

The fetched source stays untouched. CMake replaces only these compilation units.
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
# Build-tree core overlay: every translation unit sees the same extended header.
# Only the pinned core directory is read; the downloaded checkout is untouched.
for source in (a.source / 'core').rglob('*'):
    if not source.is_file():
        continue
    relative = source.relative_to(a.source)
    contents = source.read_bytes()
    if relative.as_posix() == 'core/puppet.hpp':
        contents = replace(contents.decode('utf-8'), '    // queries', '''    // Original D mutable collection queries, used by SceneItem attachment.
    auto& getRootParts() { return rootParts; }
    auto& getDrivers() { return drivers; }
    // queries''').encode('utf-8')
    elif relative.as_posix() == 'core/nodes/node.cpp':
        code=contents.decode('utf-8', errors='surrogateescape')
        code=replace_function(code,'std::array<float, 4> Node::getCombinedBounds(','''std::array<float, 4> Node::getCombinedBounds(bool reupdate, bool countPuppet) {
    auto combined = getInitialBoundsSize();
    bool hasCombined = false;
    if (auto drawable = std::dynamic_pointer_cast<Drawable>(shared_from_this())) {
        if (reupdate) drawable->updateBounds();
        if (!drawable->vertices.empty() || children.empty()) {
            combined = drawable->bounds.value_or(std::array<float,4>{NAN,NAN,NAN,NAN});
            hasCombined = true;
        }
    } else { hasCombined = children.empty(); }
    for (auto child : children) {
        auto b = child->getCombinedBounds(reupdate);
        if (!hasCombined) { combined=b; hasCombined=true; }
        else {
            if(b[0]<combined[0])combined[0]=b[0];
            if(b[1]<combined[1])combined[1]=b[1];
            if(b[2]>combined[2])combined[2]=b[2];
            if(b[3]>combined[3])combined[3]=b[3];
        }
    }
    if(countPuppet) {
        auto matrix=puppetRef()->transform.matrix();
        auto tl=matrix.transformPoint({combined[0],combined[1],0});
        auto br=matrix.transformPoint({combined[2],combined[3],0});
        return {tl.x,tl.y,br.x,br.y};
    }
    return combined;
}''')
        contents=code.encode('utf-8', errors='surrogateescape')
    elif relative.as_posix() == 'core/nodes/drawable.hpp':
        contents=replace(contents.decode('utf-8'), '    virtual void finalizeDrawable();', '    void finalize() override;\n    virtual void finalizeDrawable();').encode('utf-8')
    elif relative.as_posix() == 'core/nodes/drawable.cpp':
        code=contents.decode('utf-8', errors='surrogateescape')
        code=replace(code, 'void Drawable::finalizeDrawable() {', '''void Drawable::finalize() {
    Deformable::finalize();
    finalizeDrawable();
    setupSelf();
}

void Drawable::finalizeDrawable() {''')
        # Resolve endpoints before the first update without rewriting authored mappings.
        # Do not import the D loader's reciprocal/vertex-count based seam repair.
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
        code=replace_function(code,'void Drawable::updateBounds()', '''void Drawable::updateBounds() {
    if (!gDoGenerateBounds) return;
    auto matrix=getDynamicMatrix();
    if(vertices.empty()) {
        auto transform=this->transform();
        bounds=std::array<float,4>{transform.translation.x,transform.translation.y,transform.translation.x,transform.translation.y};
        return;
    }
    auto point=[&](size_t i){auto v=vertices.at(i);auto d=deformation.at(i);return matrix.transformPoint({v.x+d.x,v.y+d.y,0});};
    auto first=point(0);
    std::array<float,4> b{first.x,first.y,first.x,first.y};
    for(size_t i=1;i<vertices.size();++i){auto v=point(i);if(v.x<b[0])b[0]=v.x;if(v.y<b[1])b[1]=v.y;if(v.x>b[2])b[2]=v.x;if(v.y>b[3])b[3]=v.y;}
    bounds=b;
}''')
        contents=code.encode('utf-8', errors='surrogateescape')
    destination = a.output / relative
    destination.parent.mkdir(parents=True, exist_ok=True)
    if not destination.exists() or destination.read_bytes() != contents:
        destination.write_bytes(contents)
code = (a.source / 'core/texture.cpp').read_text(encoding='utf-8')
code = '#include "nicxlive/native_backend.hpp"\n' + code
code = '#include <stdexcept>\n' + code
# Global/TLS textures can outlive the deferred-disposal queue during C++ exit.
# Keep normal frame disposal unchanged, but never append to destroyed storage.
code = '#include <atomic>\n' + code
code = replace(code, 'std::vector<PendingTextureDisposal> gPendingTextureDisposals;', '''std::atomic<bool> gPendingTextureDisposalsAlive{true};
struct PendingTextureDisposalQueue : std::vector<PendingTextureDisposal> {
    ~PendingTextureDisposalQueue() { gPendingTextureDisposalsAlive.store(false); }
};
PendingTextureDisposalQueue gPendingTextureDisposals;''')
code = replace(code, 'void Texture::disposeDeferred() {', '''void Texture::disposeDeferred() {
    if (!gPendingTextureDisposalsAlive.load()) {
        backendId_ = 0;
        backend_.reset();
        return;
    }''')
# ShallowTexture retains decoded channels; conversion happens at GPU upload.
code = replace(code, 'auto img = loadImage(file, w, h, comp, channels);\n    data = img;', 'auto img = loadImage(file, w, h, comp, 0);\n    if (img.empty()) throw std::runtime_error("Cannot decode image: " + file);\n    data = img;')
code = replace(code, 'channels = channels == 0 ? comp : channels;\n    convChannels = channels;', 'this->channels = comp;\n    convChannels = channels == 0 ? comp : channels;')
code = replace(code, 'auto img = loadImageBuffer(buffer, w, h, comp, channels);', 'auto img = loadImageBuffer(buffer, w, h, comp, 0);\n    if (img.empty()) throw std::runtime_error("Cannot decode image buffer");')
code = replace(code, 'this->channels = channels == 0 ? comp : channels;\n    convChannels = this->channels;', 'this->channels = comp;\n    convChannels = channels == 0 ? comp : channels;')
code = replace(code, 'backendId_ = backend->createTexture(data_, width_, height_, outChannels, stencil_);', '''if (auto native = dynamic_cast<NativeBackend*>(backend.get())) {
            backendId_ = native->createNativeTexture(data_, width_, height_, inChannels, outChannels, stencil_, useMipmaps_);
        } else {
            backendId_ = backend->createTexture(data_, width_, height_, outChannels, stencil_);
        }''')
for method, call in [('setFiltering(Filtering mode)', 'setNativeTextureFiltering(backendId_, mode, useMipmaps_)'), ('setWrapping(Wrapping mode)', 'setNativeTextureWrapping(backendId_, mode)'), ('setAnisotropy(float val)', 'setNativeTextureAnisotropy(backendId_, val)')]:
    before = 'void Texture::' + method + ' {'
    code = replace(code, before, before + '''
    if (auto backend = backend_.lock()) {
        if (auto native = dynamic_cast<NativeBackend*>(backend.get())) {
            if (backendId_) native->''' + call + ''';
            return;
        }
    }''')
code = replace(code, 'void Texture::setData(const std::vector<uint8_t>& data, int inChannels) {', '''void Texture::setData(const std::vector<uint8_t>& data, int inChannels) {
    if (auto backend = backend_.lock()) {
        if (auto native = dynamic_cast<NativeBackend*>(backend.get())) {
            const int actualChannels = inChannels == -1 ? channels_ : inChannels;
            if (locked_) { lockedData_ = data; modified_ = true; }
            else if (backendId_) { native->setNativeTextureData(backendId_, data, actualChannels); }
            return;
        }
    }''')
code = replace(code, 'void Texture::dispose() {', '''void Texture::dispose() {
    if (auto backend = backend_.lock()) {
        if (dynamic_cast<NativeBackend*>(backend.get())) {
            if (backendId_) backend->disposeTexture(backendId_);
            backendId_ = 0;
            return;
        }
    }''')
code = replace(code, 'lockedData_ = data_;', '''if (auto backend = backend_.lock(); backend && dynamic_cast<NativeBackend*>(backend.get())) {
        lockedData_ = backendId_ ? dynamic_cast<NativeBackend*>(backend.get())->readNativeTexture(backendId_)
            : std::vector<uint8_t>(static_cast<size_t>(width_) * height_ * channels_);
    } else { lockedData_ = data_; }''')
code = replace(code, 'void Texture::unlock() {', '''void Texture::unlock() {
    if (auto backend = backend_.lock(); backend && dynamic_cast<NativeBackend*>(backend.get())) {
        if (locked_) {
            locked_ = false;
            if (modified_) setData(lockedData_, channels_);
            modified_ = false;
            lockedData_.clear();
        }
        return;
    }''')
# Source-relative includes must resolve against the original core directory.
write('texture.cpp',code)
part = (a.source / 'core/nodes/part.cpp').read_text(encoding='utf-8')
part = '#include "nicxlive/native_backend.hpp"\n' + part
part = replace(part, 'packet.useMultistageBlend = useMultistageBlend(blendMode);', '''if (auto backend = ::nicxlive::core::getCurrentRenderBackend(); backend && dynamic_cast<::nicxlive::core::NativeBackend*>(backend.get())) {
        packet.useMultistageBlend = dynamic_cast<::nicxlive::core::NativeBackend*>(backend.get())->useNativeMultistageBlend(blendMode);
    } else { packet.useMultistageBlend = useMultistageBlend(blendMode); }''')
write('part.cpp',part)
puppet = (a.source / 'core/puppet.cpp').read_text(encoding='utf-8')
puppet = '#include "nicxlive/native_backend.hpp"\n' + puppet
puppet = replace(puppet, 'auto backendQueue = std::make_shared<render::QueueRenderBackend>();', '''auto currentNative = getCurrentRenderBackend();
    auto backendQueue = std::make_shared<render::QueueRenderBackend>();''')
puppet = replace(puppet, '    forceFullRebuild = true;\n}', '''    if (currentNative && dynamic_cast<NativeBackend*>(currentNative.get())) setRenderBackend(currentNative);
    forceFullRebuild = true;
}''')
puppet = replace(puppet, '    if (auto queueBackend = std::dynamic_pointer_cast<render::QueueRenderBackend>(renderBackend)) {', '''    if (auto native = dynamic_cast<NativeBackend*>(renderBackend.get())) {
        commandEmitterOwned = native->createNativeCommandEmitter();
        commandEmitterRaw = commandEmitterOwned.get();
    }
    if (auto queueBackend = std::dynamic_pointer_cast<render::QueueRenderBackend>(renderBackend)) {''')
write('puppet.cpp',puppet)
