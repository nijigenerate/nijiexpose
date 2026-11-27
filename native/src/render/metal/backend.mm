// Copyright 2024 nijigenerate Project. BSD-2-Clause; see LICENSE.
// Packet construction follows the editor's Vulkan backend; encoding uses Metal directly.
#include "backend.hpp"
#include "context_internal.hpp"
#include "render/settings.hpp"
#include <bit>
#include <core/render/backend_queue.hpp>
#include <core/render/shared_deform_buffer.hpp>
#include <core/runtime_state.hpp>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <set>
namespace nijiexpose::metal {
namespace core=nicxlive::core;
using namespace nicxlive::core::nodes;
namespace {
struct Vertex { float x,y,u,v,z=0; };
struct Material { float tint[4]{1,1,1,1}; float screen[4]{}; float emission[4]{}; };
struct DrawParameters { Material material; uint32_t mode,kind,offscreen,padding=0; };
static_assert(sizeof(DrawParameters)==64);
Vec3 transformClip(const Mat4 &m, const Vec3 &v) {
  float in[4] = {v.x, v.y, v.z, 1}, out[4]{};
  for (int row = 0; row < 4; ++row)
    for (int col = 0; col < 4; ++col)
      out[row] += m[row][col] * in[col];
  float inv = out[3] == 0 || std::isnan(out[3]) ? 1.f : 1.f / out[3];
  return {out[0] * inv, out[1] * inv, out[2] * inv};
}
bool advanced(BlendMode m) {
  auto n = int(m);
  return (n >= 1 && n <= 6) || (n >= 9 && n <= 13);
}
void blendState(MTLRenderPipelineColorAttachmentDescriptor* s,BlendMode mode) {
    s.blendingEnabled=YES;
    s.sourceRGBBlendFactor=s.sourceAlphaBlendFactor=MTLBlendFactorOne;
    s.destinationRGBBlendFactor=s.destinationAlphaBlendFactor=MTLBlendFactorOneMinusSourceAlpha;
    switch(mode) {
    case BlendMode::Multiply: s.sourceRGBBlendFactor=MTLBlendFactorDestinationColor; s.sourceAlphaBlendFactor=MTLBlendFactorDestinationAlpha; break;
    case BlendMode::Screen: s.destinationRGBBlendFactor=MTLBlendFactorOneMinusSourceColor; s.destinationAlphaBlendFactor=MTLBlendFactorOneMinusSourceAlpha; break;
    case BlendMode::Darken: s.rgbBlendOperation=MTLBlendOperationMin; s.destinationRGBBlendFactor=s.destinationAlphaBlendFactor=MTLBlendFactorOne; break;
    case BlendMode::Lighten: s.rgbBlendOperation=MTLBlendOperationMax; s.destinationRGBBlendFactor=s.destinationAlphaBlendFactor=MTLBlendFactorOne; break;
    case BlendMode::ColorDodge: s.sourceRGBBlendFactor=MTLBlendFactorDestinationColor; s.sourceAlphaBlendFactor=MTLBlendFactorDestinationAlpha; s.destinationRGBBlendFactor=s.destinationAlphaBlendFactor=MTLBlendFactorOne; break;
    case BlendMode::LinearDodge: s.destinationRGBBlendFactor=MTLBlendFactorOneMinusSourceColor; break;
    case BlendMode::AddGlow: s.destinationRGBBlendFactor=MTLBlendFactorOne; break;
    case BlendMode::Exclusion: s.sourceRGBBlendFactor=MTLBlendFactorOneMinusDestinationColor; s.destinationRGBBlendFactor=MTLBlendFactorOneMinusSourceColor; s.destinationAlphaBlendFactor=MTLBlendFactorOne; break;
    case BlendMode::Subtract: s.rgbBlendOperation=MTLBlendOperationReverseSubtract; s.sourceRGBBlendFactor=MTLBlendFactorOneMinusDestinationColor; s.sourceAlphaBlendFactor=MTLBlendFactorOneMinusDestinationAlpha; s.destinationRGBBlendFactor=s.destinationAlphaBlendFactor=MTLBlendFactorOne; break;
    case BlendMode::Inverse: s.sourceRGBBlendFactor=MTLBlendFactorOneMinusDestinationColor; s.sourceAlphaBlendFactor=MTLBlendFactorOneMinusDestinationAlpha; break;
    case BlendMode::DestinationIn: s.sourceRGBBlendFactor=s.sourceAlphaBlendFactor=MTLBlendFactorZero; s.destinationRGBBlendFactor=s.destinationAlphaBlendFactor=MTLBlendFactorSourceAlpha; break;
    case BlendMode::ClipToLower: s.sourceRGBBlendFactor=s.sourceAlphaBlendFactor=MTLBlendFactorDestinationAlpha; break;
    case BlendMode::SliceFromLower: s.sourceRGBBlendFactor=s.sourceAlphaBlendFactor=MTLBlendFactorZero; break;
    default: break;
    }
}
}
struct Backend::Impl {
    Context& c=context();
    struct Texture {
        Image image;
        id<MTLSamplerState> sampler;
        bool mipmaps=true,uploaded=false;
        bool alphaEmpty=false;
    };
    struct Target {
        std::array<uint32_t,3> colors{};
        Image stencil;
        Image tileStencil;
        uint32_t width{},height{};
        unsigned validOutputs=7;
    };
  struct Batch {
    Target *target{};
    uint32_t first{}, count{};
    int32_t base{};
    std::array<uint32_t, 3> textures{};
    Material material;
    BlendMode blend = BlendMode::Normal;
    int mask = 0;
    int kind = 0;
    uint32_t reference = 1;
    int clearStencil = -1;
    bool clear = false;
    bool depth = false;
  };
    std::map<uint32_t,Texture> textures;
    std::map<core::RenderResourceHandle,std::vector<uint16_t>> indices;
    uint32_t nextTexture=1,nextIndex=1;
    std::unique_ptr<Target> root,blurA,blurB;
    struct Composite { std::weak_ptr<core::DynamicCompositeSurface> owner; std::unique_ptr<Target> target; };
    std::map<const core::DynamicCompositeSurface*,Composite> composites;
    std::vector<Vec3> debugPositions;
    std::vector<uint16_t> debugIndices;
    float pointSize=4,lineWidth=1;
    std::vector<Target*> stack;
    Target* target{};
    std::vector<Vertex> vertices;
    std::vector<uint16_t> elements;
    std::vector<Batch> batches;
    std::vector<Vertex> archivedVertices;
    std::vector<uint16_t> archivedElements;
    std::vector<Batch> archivedBatches;
    bool archiveValid=false,partialScene=false,replaying=false;
    std::set<uint32_t> externalOutputs;
    uint32_t white{},black{};
    int mask=0;
    uint32_t reference=1;
    bool lighting=false,postprocessingRequested=false,scenePending=false,sceneEnded=false;
    bool maskWritten=false,blurRequested=false;
    struct Pipeline { id<MTLRenderPipelineState> render; id<MTLDepthStencilState> depth; };
    std::map<uint64_t,Pipeline> pipelines;
    id<MTLSamplerState> sampler;
    const bool profile=[] { auto s=std::getenv("NIJIEXPOSE_METAL_PROFILE"); return s && std::strcmp(s,"0"); }();
    const bool referenceRendering=[] {
        auto s=std::getenv("NIJIEXPOSE_METAL_REFERENCE"); return s && std::strcmp(s,"0");
    }();
    uint64_t profileFrames=0,profileDraws=0,profilePasses=0,profileColorBytes=0,profileMrtDraws=0;
    void initialize() {
        auto d=[MTLSamplerDescriptor new]; d.minFilter=d.magFilter=MTLSamplerMinMagFilterLinear;
        d.mipFilter=MTLSamplerMipFilterLinear; d.sAddressMode=d.tAddressMode=MTLSamplerAddressModeClampToEdge;
        sampler=[c.p->device newSamplerStateWithDescriptor:d];
    }
    unsigned availableOutputs(const Target& t) const {
        unsigned mask=1;
        if(t.colors[1] && t.colors[1]!=t.colors[0]) mask|=2;
        if(t.colors[2] && t.colors[2]!=t.colors[0] && t.colors[2]!=t.colors[1]) mask|=4;
        return mask;
    }
    Target* producer(uint32_t id) const {
        auto match=[&](Target* t) { return t && std::find(t->colors.begin(),t->colors.end(),id)!=t->colors.end(); };
        if(match(root.get())) return root.get();
        for(const auto& [_,entry]:composites) if(match(entry.target.get())) return entry.target.get();
        for(auto* t:{blurA.get(),blurB.get()}) if(match(t)) return t;
        return nullptr;
    }
    void replayOutputs() {
        if(!archiveValid || replaying) return;
        bool missing=false;
        for(const auto& b:archivedBatches)
            missing|=(b.target->validOutputs&availableOutputs(*b.target))!=availableOutputs(*b.target);
        if(!missing) return;
        replaying=true;
        vertices.swap(archivedVertices); elements.swap(archivedElements); batches.swap(archivedBatches);
        try { c.immediate([&] { record(true); }); }
        catch(...) {
            vertices.swap(archivedVertices); elements.swap(archivedElements); batches.swap(archivedBatches);
            replaying=false; throw;
        }
        vertices.swap(archivedVertices); elements.swap(archivedElements); batches.swap(archivedBatches);
        replaying=false;
    }
    void beforeMutation() {
        // Materialize omitted outputs while the archived texture inputs and
        // target addresses still describe the last submitted frame.
        replayOutputs(); archiveValid=false;
        archivedVertices.clear(); archivedElements.clear(); archivedBatches.clear();
    }
    void requireTexture(uint32_t id) {
        if(auto* t=producer(id)) {
            unsigned bits=0;
            for(unsigned i=0;i<3;++i) if(t->colors[i]==id) bits|=1u<<i;
            if(bits&1) return;
            externalOutputs.insert(id);
            if((t->validOutputs&bits)!=bits) replayOutputs();
        }
    }
    void destroyTarget(Target& t) { beforeMutation(); t={}; }
    std::unique_ptr<Target> makeTarget(const std::array<uint32_t,3>& ids) {
        auto t=std::make_unique<Target>(); t->colors=ids;
        const auto& image=textures.at(ids[0]).image; t->width=image.width; t->height=image.height;
        for(auto id:ids) if(id) textures.at(id).alphaEmpty=false;
        t->stencil=c.createImage(t->width,t->height,1,true); return t;
    }
    Pipeline pipeline(const Batch& b,unsigned outputs) {
        const bool mrt=outputs!=1;
        const bool screened=b.material.screen[0]!=0 || b.material.screen[1]!=0 || b.material.screen[2]!=0;
        const bool emissive=b.textures[1] && b.textures[1]!=black;
        const bool bumped=b.textures[2] && b.textures[2]!=black;
        uint64_t key=uint64_t(b.blend)|(uint64_t(b.kind)<<8)|(uint64_t(b.mask)<<16)|
            (uint64_t(b.depth)<<20)|(uint64_t(outputs)<<21)|(uint64_t(screened)<<24)|
            (uint64_t(emissive)<<25)|(uint64_t(bumped)<<26);
        if(auto it=pipelines.find(key);it!=pipelines.end()) return it->second;
        auto d=[MTLRenderPipelineDescriptor new];
        d.label=@"Model geometry";
        d.vertexFunction=[c.p->library newFunctionWithName:@"sceneVertex"];
        NSString* fragment=mrt?@"sceneFragment":@"singleFragment";
        if(!referenceRendering) {
            if(b.mask==1) fragment=b.kind==8?@"stencilFragment":@"maskFragment";
            else if(b.kind==3 || b.kind==7)
                fragment=mrt?@"visibleMaskPlanesFragment":@"visibleMaskColorFragment";
            else if(b.kind==0 || b.kind==5) {
                if(mrt) fragment=advanced(b.blend)?@"blendPlanesFragment":@"planesFragment";
                else fragment=advanced(b.blend)?@"blendColorFragment":@"colorFragment";
            }
        }
        auto constants=[MTLFunctionConstantValues new];
        bool emission=(outputs&2)!=0,bump=(outputs&4)!=0;
        bool lightingFetch=!referenceRendering && c.advancedBlendSupported;
        uint32_t equation=uint32_t(b.blend);
        [constants setConstantValue:&emission type:MTLDataTypeBool atIndex:0];
        [constants setConstantValue:&screened type:MTLDataTypeBool atIndex:1];
        [constants setConstantValue:&equation type:MTLDataTypeUInt atIndex:2];
        [constants setConstantValue:&emissive type:MTLDataTypeBool atIndex:3];
        [constants setConstantValue:&bumped type:MTLDataTypeBool atIndex:4];
        [constants setConstantValue:&lightingFetch type:MTLDataTypeBool atIndex:5];
        [constants setConstantValue:&bump type:MTLDataTypeBool atIndex:6];
        NSError* shaderError=nil;
        d.fragmentFunction=[c.p->library newFunctionWithName:fragment constantValues:constants error:&shaderError];
        if(!d.fragmentFunction) throw std::runtime_error(shaderError.localizedDescription.UTF8String);
        auto vd=[MTLVertexDescriptor vertexDescriptor];
        vd.attributes[0].format=MTLVertexFormatFloat2; vd.attributes[0].offset=0; vd.attributes[0].bufferIndex=0;
        vd.attributes[1].format=MTLVertexFormatFloat2; vd.attributes[1].offset=8; vd.attributes[1].bufferIndex=0;
        vd.attributes[2].format=MTLVertexFormatFloat; vd.attributes[2].offset=16; vd.attributes[2].bufferIndex=0;
        vd.layouts[0].stride=sizeof(Vertex); d.vertexDescriptor=vd;
        d.depthAttachmentPixelFormat=MTLPixelFormatDepth32Float_Stencil8;
        d.stencilAttachmentPixelFormat=MTLPixelFormatDepth32Float_Stencil8;
        for(unsigned i=0;i<3;++i) {
            if(!(outputs&(1u<<i))) continue;
            auto a=d.colorAttachments[i]; a.pixelFormat=MTLPixelFormatRGBA8Unorm;
            blendState(a,b.blend);
            if(i==0 && advanced(b.blend)) a.blendingEnabled=NO;
            a.writeMask=b.mask==1?MTLColorWriteMaskNone:MTLColorWriteMaskAll;
        }
        auto a=d.colorAttachments[0];
        if(b.kind==9 || b.kind==13) a.blendingEnabled=NO;
        if(b.kind==10) {
            a.sourceRGBBlendFactor=MTLBlendFactorDestinationColor; a.destinationRGBBlendFactor=MTLBlendFactorZero;
            a.sourceAlphaBlendFactor=MTLBlendFactorZero; a.destinationAlphaBlendFactor=MTLBlendFactorOne;
        }
        if(b.kind==11) {
            a.sourceRGBBlendFactor=a.destinationRGBBlendFactor=MTLBlendFactorOne;
            a.sourceAlphaBlendFactor=MTLBlendFactorZero; a.destinationAlphaBlendFactor=MTLBlendFactorOne;
        }
        NSError* error=nil; Pipeline result;
        result.render=[c.p->device newRenderPipelineStateWithDescriptor:d error:&error];
        if(!result.render) throw std::runtime_error(error.localizedDescription.UTF8String);
        auto ds=[MTLDepthStencilDescriptor new];
        ds.depthCompareFunction=b.depth?MTLCompareFunctionLess:MTLCompareFunctionAlways;
        ds.depthWriteEnabled=b.depth;
        if(b.mask) {
            auto stencil=[MTLStencilDescriptor new];
            stencil.stencilCompareFunction=b.mask==1?MTLCompareFunctionAlways:MTLCompareFunctionEqual;
            stencil.depthStencilPassOperation=b.mask==1?MTLStencilOperationReplace:MTLStencilOperationKeep;
            stencil.readMask=255; stencil.writeMask=b.mask==1?255:0;
            ds.frontFaceStencil=ds.backFaceStencil=stencil;
        }
        result.depth=[c.p->device newDepthStencilStateWithDescriptor:ds];
        pipelines.emplace(key,result); return result;
    }
    std::map<Target*,unsigned> planOutputs(bool full) {
        std::map<Target*,unsigned> result;
        for(const auto& b:batches) {
            auto& mask=result[b.target]; mask|=1;
            if(full) mask|=availableOutputs(*b.target);
            for(unsigned i=1;i<3;++i)
                if(externalOutputs.contains(b.target->colors[i])) mask|=(1u<<i)&availableOutputs(*b.target);
        }
        auto require=[&](uint32_t texture) {
            if(!texture) return false;
            auto* t=producer(texture); if(!t) return false;
            unsigned bits=0;
            for(unsigned i=0;i<3;++i) if(t->colors[i]==texture) bits|=1u<<i;
            bits&=availableOutputs(*t);
            auto& mask=result[t]; const auto old=mask; mask|=bits; return old!=mask;
        };
        bool changed;
        do {
            changed=false;
            for(const auto& b:batches) if(b.count) {
                changed|=require(b.textures[0]);
                if((b.kind<9 && b.mask!=1 && (result[b.target]&2)) || b.kind==13 || b.kind==2)
                    changed|=require(b.textures[1]);
                if(b.kind<9 && b.mask!=1 && (result[b.target]&4)) changed|=require(b.textures[2]);
            }
        } while(changed);
        return result;
    }
    bool needsMipmaps(const Batch& b,unsigned outputs) const {
        if(b.kind!=14) return false;
        for(unsigned i=0;i<3;++i) if((outputs&(1u<<i)) && b.target->colors[i] &&
            textures.at(b.target->colors[i]).image.mipLevels>1) return true;
        return false;
    }
    bool invisible(const Batch& b) const {
        if(referenceRendering || !b.count || b.kind>=9) return false;
        if(b.kind!=8) {
            auto it=textures.find(b.textures[0]);
            const bool alphaDiscard=b.kind==0 || b.kind==5 ||
                ((b.kind==3 || b.kind==6 || b.kind==7) && b.material.screen[3]>=0);
            if(alphaDiscard && it!=textures.end() && it->second.alphaEmpty) return true;
        }
        bool left=true,right=true,top=true,bottom=true;
        for(uint32_t i=0;i<b.count;++i) {
            const auto& v=vertices.at(size_t(b.base)+elements.at(b.first+i));
            if(!std::isfinite(v.x) || !std::isfinite(v.y)) return false;
            left&=v.x < -1; right&=v.x > 1; top&=v.y > 1; bottom&=v.y < -1;
        }
        return left || right || top || bottom;
    }
    void record(bool forceFull=false) {
        if(!c.recording()) throw std::logic_error("Metal recording requires a command buffer");
        const bool full=forceFull || referenceRendering || partialScene || !sceneEnded;
        const auto outputs=planOutputs(full);
        const auto planesFor=[&](const Batch& b) { return b.kind<9?outputs.at(b.target):1u; };
        std::map<Target*,unsigned> segments;
        std::map<Target*,bool> startsClear;
        Target* previous=nullptr; unsigned previousPlanes=0;
        for(const auto& b:batches) {
            if(b.kind==14) {
                if(referenceRendering || needsMipmaps(b,outputs.at(b.target))) previous=nullptr;
                continue;
            }
            const auto planes=planesFor(b);
            if(previous!=b.target || previousPlanes!=planes || b.clear) {
                if(!segments.contains(b.target)) startsClear[b.target]=b.clear;
                ++segments[b.target]; previous=b.target; previousPlanes=planes;
            }
        }
        auto command=c.p->command;
        id<MTLBuffer> vertexBuffer=nil,indexBuffer=nil;
        if(!vertices.empty()) vertexBuffer=[c.p->device newBufferWithBytes:vertices.data() length:vertices.size()*sizeof(Vertex) options:MTLResourceStorageModeShared];
        if(!elements.empty()) indexBuffer=[c.p->device newBufferWithBytes:elements.data() length:elements.size()*sizeof(uint16_t) options:MTLResourceStorageModeShared];
        Target* active=nullptr; unsigned activePlanes=0;
        bool activeMemoryless=false;
        size_t nextBatch=0;
        id<MTLRenderCommandEncoder> encoder=nil;
        uint64_t draws=0,passes=0,colorBytes=0,mrtDraws=0;
        auto end=[&] {
            if(!encoder) return;
            if(!referenceRendering) {
                bool depthNeeded=partialScene,stencilNeeded=partialScene,stencilCleared=false;
                for(size_t i=nextBatch;i<batches.size();++i) {
                    const auto& future=batches[i];
                    if(future.target!=active || future.kind==14) continue;
                    if(future.clear) break;
                    depthNeeded|=future.depth;
                    if(future.clearStencil>=0) stencilCleared=true;
                    if(!stencilCleared && future.mask) stencilNeeded=true;
                }
                [encoder setDepthStoreAction:depthNeeded && !activeMemoryless?MTLStoreActionStore:MTLStoreActionDontCare];
                [encoder setStencilStoreAction:stencilNeeded && !activeMemoryless?MTLStoreActionStore:MTLStoreActionDontCare];
            }
            [encoder endEncoding]; encoder=nil; active=nullptr;
        };
        for(size_t index=0;index<batches.size();++index) {
            nextBatch=index;
            const auto& b=batches[index];
            auto& t=*b.target;
            if(b.kind==14) {
                if(referenceRendering || needsMipmaps(b,outputs.at(&t))) {
                    end();
                    for(unsigned i=0;i<3;++i) if((outputs.at(&t)&(1u<<i)) && t.colors[i])
                        c.generateMipmaps(textures.at(t.colors[i]).image);
                }
                continue;
            }
            const auto planes=planesFor(b);
            Image lightingSnapshot;
            if((b.kind==13 && (referenceRendering || !c.advancedBlendSupported)) ||
                (b.count && advanced(b.blend) && !c.advancedBlendSupported)) {
                end();
                lightingSnapshot=c.createImage(t.width,t.height);
                auto blit=[command blitCommandEncoder];
                [blit copyFromTexture:native(textures.at(t.colors[0]).image) sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0,0,0) sourceSize:MTLSizeMake(t.width,t.height,1) toTexture:native(lightingSnapshot) destinationSlice:0 destinationLevel:0 destinationOrigin:MTLOriginMake(0,0,0)];
                [blit endEncoding];
            }
            const bool restart=active!=&t || activePlanes!=planes || b.clear;
            if(restart) {
                end(); auto pass=[MTLRenderPassDescriptor renderPassDescriptor];
                for(unsigned i=0;i<3;++i) {
                    if(!(planes&(1u<<i))) continue;
                    auto a=pass.colorAttachments[i]; a.texture=native(textures.at(t.colors[i]).image);
                    a.loadAction=b.clear?MTLLoadActionClear:MTLLoadActionLoad; a.storeAction=MTLStoreActionStore;
                    Vec4 color{};
                    if(i==0 && &t==root.get() && !render::transparentWindow)
                        core::inGetClearColor(color.x,color.y,color.z,color.w);
                    a.clearColor=MTLClearColorMake(color.x,color.y,color.z,color.w);
                    colorBytes+=uint64_t(t.width)*t.height*4;
                }
                activeMemoryless=!referenceRendering && !partialScene && sceneEnded &&
                    c.advancedBlendSupported && segments.at(&t)==1 && startsClear.at(&t);
                if(activeMemoryless && !t.tileStencil.owner) t.tileStencil=c.createImage(t.width,t.height,1,true,true);
                pass.depthAttachment.texture=pass.stencilAttachment.texture=native(activeMemoryless?t.tileStencil:t.stencil);
                bool usesDepth=false,usesStencil=false;
                for(size_t j=index;j<batches.size();++j) {
                    const auto& next=batches[j];
                    if(next.target!=&t || next.kind==14) continue;
                    if(j!=index && next.clear) break;
                    usesDepth|=next.depth; usesStencil|=next.mask!=0 || next.clearStencil>=0;
                }
                pass.depthAttachment.loadAction=b.clear?MTLLoadActionClear:
                    !referenceRendering && !usesDepth?MTLLoadActionDontCare:MTLLoadActionLoad;
                pass.depthAttachment.storeAction=referenceRendering?MTLStoreActionStore:MTLStoreActionUnknown;
                pass.depthAttachment.clearDepth=1;
                pass.stencilAttachment.loadAction=b.clear || b.clearStencil>=0?MTLLoadActionClear:
                    !referenceRendering && !usesStencil?MTLLoadActionDontCare:MTLLoadActionLoad;
                pass.stencilAttachment.storeAction=referenceRendering?MTLStoreActionStore:MTLStoreActionUnknown;
                pass.stencilAttachment.clearStencil=std::max(0,b.clearStencil);
                encoder=[command renderCommandEncoderWithDescriptor:pass];
                encoder.label=@"Model target";
                [encoder setViewport:MTLViewport{0,0,double(t.width),double(t.height),0,1}];
                [encoder setScissorRect:MTLScissorRect{0,0,t.width,t.height}];
                active=&t; activePlanes=planes; ++passes;
            }
            if(b.clear) t.validOutputs=planes;
            else if(b.count && b.mask!=1) t.validOutputs|=planes;
            if(!restart && b.clearStencil>=0) {
                // Clear stencil inside the existing tile pass. Restarting an
                // encoder here would store/reload all three color planes for
                // every mask, despite changing no color pixels.
                Batch clear=b; clear.kind=8; clear.mask=1; clear.depth=false;
                clear.blend=BlendMode::Normal;
                const auto state=pipeline(clear,planes);
                [encoder setRenderPipelineState:state.render];
                [encoder setDepthStencilState:state.depth];
                [encoder setStencilReferenceValue:uint32_t(b.clearStencil)];
                const Vertex triangle[]={{-1,-1,0,0},{3,-1,0,0},{-1,3,0,0}};
                const DrawParameters params{Material{},0,8,uint32_t(&t!=root.get())};
                [encoder setVertexBytes:triangle length:sizeof(triangle) atIndex:0];
                [encoder setVertexBytes:&params length:sizeof(params) atIndex:1];
                [encoder setFragmentBytes:&params length:sizeof(params) atIndex:0];
                for(unsigned i=0;i<4;++i) {
                    [encoder setFragmentTexture:native(textures.at(white).image) atIndex:i];
                    [encoder setFragmentSamplerState:sampler atIndex:i];
                }
                [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
                ++draws;
            }
            if(!b.count || invisible(b)) continue;
            const auto state=pipeline(b,planes);
            [encoder setRenderPipelineState:state.render]; [encoder setDepthStencilState:state.depth];
            [encoder setStencilReferenceValue:b.reference];
            DrawParameters params{b.material,uint32_t(b.blend),uint32_t(b.kind),uint32_t(&t!=root.get())};
            [encoder setVertexBytes:&params length:sizeof(params) atIndex:1];
            [encoder setFragmentBytes:&params length:sizeof(params) atIndex:0];
            for(unsigned i=0;i<3;++i) {
                auto id=b.textures[i]; if(!id || !textures.contains(id)) id=i==0?white:black;
                const auto& texture=textures.at(id);
                [encoder setFragmentTexture:i==0 && b.kind==13 && lightingSnapshot.owner?native(lightingSnapshot):native(texture.image) atIndex:i];
                [encoder setFragmentSamplerState:texture.sampler?texture.sampler:sampler atIndex:i];
            }
            [encoder setFragmentTexture:lightingSnapshot.owner?native(lightingSnapshot):native(textures.at(black).image) atIndex:3];
            [encoder setVertexBuffer:vertexBuffer offset:size_t(b.base)*sizeof(Vertex) atIndex:0];
            [encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:b.count indexType:MTLIndexTypeUInt16 indexBuffer:indexBuffer indexBufferOffset:size_t(b.first)*sizeof(uint16_t)];
            ++draws;
            if(planes!=1) ++mrtDraws;
        }
        nextBatch=batches.size();
        end();
        if(profile) {
            profileDraws+=draws; profilePasses+=passes; profileColorBytes+=colorBytes; profileMrtDraws+=mrtDraws;
            if(++profileFrames==120) {
                std::fprintf(stderr,"[metal-gpu] draws=%.1f passes=%.1f mrt_draws=%.1f color_store_MiB=%.2f framebuffer_fetch=%d reference=%d\n",
                    profileDraws/120.,profilePasses/120.,profileMrtDraws/120.,profileColorBytes/120./1048576.,
                    int(c.advancedBlendSupported),int(referenceRendering));
                profileFrames=profileDraws=profilePasses=profileColorBytes=profileMrtDraws=0;
            }
        }
    }
};
Backend &backend() {
  auto b = std::dynamic_pointer_cast<Backend>(core::getCurrentRenderBackend());
  if (!b)
    throw std::logic_error("Metal model backend is not active");
  return *b;
}
ImTextureID uiTexture(uint32_t id) { return id ? backend().uiTexture(id) : 0; }
Backend::Backend() : p(std::make_unique<Impl>()) { p->c.p->backend=this; }
Backend::~Backend() { shutdown(); }
void Backend::shutdown() {
  if(!p) return;
  p->c.p->backend=nullptr;
  p.reset();
}
void Backend::initializeRenderer() {
  p->initialize();
  p->white = createTexture({255, 255, 255, 255}, 1, 1, 4, false);
  p->black = createTexture({0, 0, 0, 0}, 1, 1, 4, false);
  resizeViewportTargets(p->c.width(), p->c.height());
}
void Backend::createDrawableBuffers(nicxlive::core::RenderResourceHandle &id) {
  if (!id)
    id = p->nextIndex++;
}
void Backend::disposeDrawableBuffers(nicxlive::core::RenderResourceHandle id) {
  if (p)
    p->indices.erase(id);
}
void Backend::uploadDrawableIndices(nicxlive::core::RenderResourceHandle id,
                                    const std::vector<uint16_t> &values) {
  p->indices[id] = values;
}
uint32_t Backend::createTexture(const std::vector<uint8_t> &data, int w, int h,
                                int channels, bool stencil) {
  return allocateTexture(data, w, h, channels, !stencil);
}
uint32_t Backend::allocateTexture(const std::vector<uint8_t>& data, int w, int h,
                                  int channels, bool mipmapped) {
  if (w == 0 && h == 0 && data.empty())
    return createTexture({0,0,0,0},1,1,4,false);
  if (w <= 0 || h <= 0 || channels < 1 || channels > 4)
    throw std::runtime_error("Invalid texture dimensions");
  auto id = p->nextTexture++;
  Impl::Texture t;
  t.mipmaps = mipmapped;
  const auto levels = mipmapped ? std::bit_width(uint32_t(std::max(w, h))) : 1u;
  t.image = p->c.createImage(w, h, levels);
  p->textures.emplace(id, std::move(t));
  try {
    updateTexture(id, data, w, h, channels);
    setTextureParams(id, core::Filtering::Linear, core::Wrapping::Clamp, 1);
  } catch (...) {
    p->textures.erase(id);
    throw;
  }
  return id;
}
void Backend::updateTexture(uint32_t id, const std::vector<uint8_t> &data,
                            int w, int h, int channels) {
  auto &t = p->textures.at(id);
  if(t.uploaded) { flush(); p->beforeMutation(); }
  if (w != int(t.image.width) || h != int(t.image.height)) {
    p->c.idle();
    p->c.destroy(t.image);
    const auto levels = t.mipmaps ? std::bit_width(uint32_t(std::max(w, h))) : 1u;
    t.image = p->c.createImage(w, h, levels);
  }
  std::vector<uint8_t> rgba(size_t(w) * h * 4, 0);
  if (!data.empty()) {
    if (data.size() < size_t(w) * h * channels)
      throw std::runtime_error("Truncated texture pixels");
    for (size_t i = 0; i < size_t(w) * h; ++i) {
      rgba[i * 4] = data[i * channels];
      rgba[i * 4 + 1] = channels > 1 ? data[i * channels + 1] : rgba[i * 4];
      rgba[i * 4 + 2] = channels > 2 ? data[i * channels + 2] : rgba[i * 4];
      rgba[i * 4 + 3] = channels > 3 ? data[i * channels + 3] : 255;
    }
  }
  p->c.upload(t.image, rgba);
  t.alphaEmpty=!p->producer(id);
  for(size_t i=3;i<rgba.size() && t.alphaEmpty;i+=4) t.alphaEmpty=rgba[i]==0;
  t.uploaded=true;
}
void Backend::setTextureParams(uint32_t id, core::Filtering filtering,
                               core::Wrapping wrapping, float anisotropy) {
    p->beforeMutation();
    auto& t=p->textures.at(id);
    auto d=[MTLSamplerDescriptor new];
    d.minFilter=d.magFilter=filtering==core::Filtering::Linear?MTLSamplerMinMagFilterLinear:MTLSamplerMinMagFilterNearest;
    d.mipFilter=t.mipmaps && filtering==core::Filtering::Linear
        ? MTLSamplerMipFilterLinear : MTLSamplerMipFilterNotMipmapped;
    // Match OpenGL's transparent GL_CLAMP_TO_BORDER, including the filtering
    // footprint at UV edges. ClampToEdge extrudes cropped Part edge texels.
    d.sAddressMode=d.tAddressMode=d.rAddressMode=wrapping==core::Wrapping::Clamp
        ? MTLSamplerAddressModeClampToZero : MTLSamplerAddressModeRepeat;
    d.maxAnisotropy=filtering==core::Filtering::Linear
        ? NSUInteger(std::clamp(anisotropy,1.f,p->c.maxAnisotropy)) : 1;
    t.sampler=[p->c.p->device newSamplerStateWithDescriptor:d];
}
void Backend::disposeTexture(uint32_t id) {
  if (!p)
    return;
  flush();
  p->beforeMutation();
  auto it = p->textures.find(id);
  if (it == p->textures.end())
    return;
  p->c.idle();
  for (auto c = p->composites.begin(); c != p->composites.end();) {
    auto &ids = c->second.target->colors;
    if (std::find(ids.begin(), ids.end(), id) != ids.end()) {
      p->destroyTarget(*c->second.target);
      c = p->composites.erase(c);
    } else
      ++c;
  }

  p->c.destroy(it->second.image);
  p->textures.erase(it);
  p->externalOutputs.erase(id);
}
std::vector<uint8_t> Backend::readTexture(uint32_t id) {
  p->requireTexture(id);
  flush();
  p->requireTexture(id);
  return p->c.read(p->textures.at(id).image);
}
ImTextureID Backend::uiTexture(uint32_t id) {
  if(!id) id=p->white;
  p->requireTexture(id);
  return p->c.texture(p->textures.at(id).image);
}
void Backend::resizeViewportTargets(int w, int h) {
  if (w <= 0 || h <= 0)
    return;
  if (p->root && p->root->width == uint32_t(w) &&
      p->root->height == uint32_t(h))
    return;
  flush();
  p->c.idle();
  if (p->root) {
    auto ids = p->root->colors;
    p->destroyTarget(*p->root);
    p->root.reset();
    for (auto id : ids)
      disposeTexture(id);
  }
  std::array<uint32_t, 3> ids;
  for (auto &id : ids) {
    id = allocateTexture({}, w, h, 4, false);
  }
  p->root = p->makeTarget(ids);
  p->target = p->root.get();
}
void Backend::beginScene() {
  flush();
  p->partialScene=false;
  p->scenePending = true;
  p->sceneEnded = false;
  p->postprocessingRequested=false;
  p->vertices.clear();
  p->elements.clear();
  p->batches.clear();
  p->stack.clear();
  p->target = p->root.get();
  p->mask = 0;
  Impl::Batch clear;
  clear.target = p->target;
  clear.clear = true;
  p->batches.push_back(clear);
  for (auto it = p->composites.begin(); it != p->composites.end();)
    if (it->second.owner.expired()) {
      p->destroyTarget(*it->second.target);
      it = p->composites.erase(it);
    } else
      ++it;
}
void Backend::endScene() { p->sceneEnded=true; }
void Backend::flush() {
  if (!p || (p->batches.empty() && !(p->scenePending && p->sceneEnded))) return;
  p->c.immediate([&] { render(); });
}
void Backend::addBasicLightingPostProcess() { p->lighting = true; }
void Backend::postProcessScene() {
  if(p->lighting) { p->postprocessingRequested=true; p->scenePending=true; p->sceneEnded=true; }
}
void Backend::render() {
  if (p->batches.empty() && !(p->scenePending && p->sceneEnded)) return;
  const bool completeScene=p->scenePending && p->sceneEnded;
  if(p->scenePending && !p->sceneEnded) p->partialScene=true;
  if (p->scenePending && p->sceneEnded) {
    // Composite emission once, independently of the ambient light toggle.
    const auto ambient = p->postprocessingRequested
        ? render::inSceneAmbientLight : Vec3{1,1,1};
    Impl::Batch light; light.target=p->root.get(); light.kind=13;
    light.base=int32_t(p->vertices.size()); light.first=uint32_t(p->elements.size()); light.count=6;
    light.textures[1]=p->root->colors[1];
    light.material.tint[0]=ambient.x; light.material.tint[1]=ambient.y; light.material.tint[2]=ambient.z;
    for(auto v:std::array<Vertex,4>{{{-1,-1,0,1},{1,-1,1,1},{1,1,1,0},{-1,1,0,0}}}) p->vertices.push_back(v);
    for(uint16_t i:{0,1,2,0,2,3}) p->elements.push_back(i);
    p->batches.push_back(light);
    p->postprocessingRequested=false;
    p->scenePending=false;
  }
  if (p->blurRequested) {
    auto append = [&](Impl::Target *target, uint32_t source, float x, float y) {
      Impl::Batch b;
      b.target = target;
      b.base = int32_t(p->vertices.size());
      b.first = uint32_t(p->elements.size());
      b.count = 6;
      b.kind = 9;
      b.textures[0] = source;
      b.material.screen[0] = x;
      b.material.screen[1] = y;
      for (auto v : std::array<Vertex, 4>{
               {{-1, -1, 0, 0}, {1, -1, 1, 0}, {1, 1, 1, 1}, {-1, 1, 0, 1}}})
        p->vertices.push_back(v);
      for (uint16_t i : {0, 1, 2, 0, 2, 3})
        p->elements.push_back(i);
      p->batches.push_back(b);
    };
    auto a = p->blurA.get(), b = p->blurB.get();
    append(a, renderImageHandle(), 0, 0);
    append(b, a->colors[0], 2.2f / a->width, 0);
    append(a, b->colors[0], 0, 2.2f / a->height);
    append(b, a->colors[0], 3.6f / a->width, 0);
    append(a, b->colors[0], 0, 3.6f / a->height);
  }
  p->record();
  p->blurRequested = false;
  if(completeScene && !p->partialScene && !p->referenceRendering) {
    p->archivedVertices.swap(p->vertices); p->archivedElements.swap(p->elements);
    p->archivedBatches.swap(p->batches); p->archiveValid=true;
  } else {
    p->archiveValid=false;
    p->archivedVertices.clear(); p->archivedElements.clear(); p->archivedBatches.clear();
  }
  p->batches.clear();
  p->vertices.clear();
  p->elements.clear();
}
ImTextureID Backend::overlayBlurTexture() {
  auto w = std::max(1u, p->root->width / 4),
       h = std::max(1u, p->root->height / 4);
  if (!p->blurA || p->blurA->width != w || p->blurA->height != h) {
    p->c.idle();
    for (auto ptr : {&p->blurA, &p->blurB}) {
      if (*ptr) {
        auto id = (*ptr)->colors[0];
        p->destroyTarget(**ptr);
        ptr->reset();
        disposeTexture(id);
      }
      auto id = allocateTexture({}, w, h, 4, false);
      *ptr = p->makeTarget({id, id, id});
    }
  }
  p->blurRequested = true;
  return uiTexture(p->blurA->colors[0]);
}
Image &Backend::sceneImage() {
  return p->textures.at(renderImageHandle()).image;
}
nicxlive::core::RenderResourceHandle Backend::renderImageHandle() { return p->root->colors[0]; }
nicxlive::core::RenderResourceHandle Backend::mainAlbedoHandle() { return p->root->colors[0]; }
nicxlive::core::RenderResourceHandle Backend::mainEmissiveHandle() {
  p->requireTexture(p->root->colors[1]); return p->root->colors[1];
}
nicxlive::core::RenderResourceHandle Backend::mainBumpHandle() {
  p->requireTexture(p->root->colors[2]); return p->root->colors[2];
}
void Backend::dumpViewport(std::vector<uint8_t> &out, int w, int h) {
  if (w != int(p->root->width) || h != int(p->root->height))
    throw std::runtime_error("Viewport readback dimensions mismatch");
  out = readTexture(renderImageHandle());
}
void Backend::drawPuppet(core::Puppet &puppet) {
  using namespace core::render;
  auto host = core::getCurrentRenderBackend();
  auto queue = std::make_shared<QueueRenderBackend>();
  puppet.setRenderBackend(queue);
  core::setCurrentRenderBackend(host);
  puppet.draw();
  for (auto &cmd : queue->queue) {
    switch (cmd.kind) {
    case RenderCommandKind::DrawPart:
      drawPartPacket(cmd.partPacket);
      break;
    case RenderCommandKind::BeginMask:
      beginMask(cmd.usesStencil);
      break;
    case RenderCommandKind::ApplyMask:
      applyMask(cmd.maskApplyPacket);
      break;
    case RenderCommandKind::BeginMaskContent:
      beginMaskContent();
      break;
    case RenderCommandKind::EndMask:
      endMask();
      break;
    case RenderCommandKind::BeginDynamicComposite:
      beginDynamicComposite(cmd.dynamicPass);
      break;
    case RenderCommandKind::EndDynamicComposite:
      endDynamicComposite(cmd.dynamicPass);
      break;
    }
  }
  queue->clear();
}
void Backend::drawPartPacket(const PartDrawPacket &packet) {
  if (!packet.renderable || !packet.vertexCount || !packet.indexCount)
    return;
  auto ix = p->indices.find(packet.indexBuffer);
  if (ix == p->indices.end())
    throw std::runtime_error("Missing Metal drawable indices");
  auto v = core::render::sharedVertexBufferData().rawStorage(),
       u = core::render::sharedUvBufferData().rawStorage(),
       d = core::render::sharedDeformBufferData().rawStorage();
  auto count = packet.vertexCount;
  if (packet.vertexOffset + packet.vertexAtlasStride + count > v.length ||
      packet.uvOffset + packet.uvAtlasStride + count > u.length ||
      packet.deformOffset + packet.deformAtlasStride + count > d.length)
    throw std::runtime_error("Drawable atlas bounds exceeded");
  Impl::Batch b;
  b.target = p->target;
  b.base = int32_t(p->vertices.size());
  b.first = uint32_t(p->elements.size());
  b.count = std::min<uint32_t>(packet.indexCount, uint32_t(ix->second.size()));
  b.mask = p->mask;
  b.reference = p->reference;
  b.blend = p->mask == 1 ? BlendMode::Normal : packet.blendMode;
  auto matrix = packet.renderMatrix * packet.modelMatrix;
  for (uint32_t i = 0; i < count; ++i) {
    float x = v.ptr[packet.vertexOffset + i] + d.ptr[packet.deformOffset + i] -
              packet.origin.x;
    float y = v.ptr[packet.vertexOffset + packet.vertexAtlasStride + i] +
              d.ptr[packet.deformOffset + packet.deformAtlasStride + i] -
              packet.origin.y;
    auto clip = transformClip(matrix, {x, y, 0});
    p->vertices.push_back({clip.x, clip.y, u.ptr[packet.uvOffset + i],
                           u.ptr[packet.uvOffset + packet.uvAtlasStride + i]});
  }
  p->elements.insert(p->elements.end(), ix->second.begin(),
                     ix->second.begin() + b.count);
  std::copy_n(packet.textureBackendIds, 3, b.textures.begin());
  if (auto node = packet.node.lock())
    for (int i = 0; i < 3; ++i)
      if (node->textures[i])
        b.textures[i] = node->textures[i]->backendId();
  b.material.tint[0] = packet.clampedTint.x;
  b.material.tint[1] = packet.clampedTint.y;
  b.material.tint[2] = packet.clampedTint.z;
  b.material.tint[3] = packet.opacity;
  b.material.screen[0] = packet.clampedScreen.x;
  b.material.screen[1] = packet.clampedScreen.y;
  b.material.screen[2] = packet.clampedScreen.z;
  b.material.screen[3] = packet.emissionStrength;
  for (const auto &[_, composite] : p->composites)
    if (composite.target->colors[0] == b.textures[0] &&
        composite.target->colors[1] == b.textures[1]) {
      b.material.emission[0] = 1;
      break;
    }
  b.kind = p->mask == 1 ? 6 : packet.isMask ? 3 : p->mask == 2 ? 5 : 0;
  if (packet.isMask || p->mask == 1)
    b.material.screen[3] = packet.maskThreshold;
  if (p->mask == 1)
    p->maskWritten = true;
  p->batches.push_back(b);
}
void Backend::beginMask(bool normal) {
  p->maskWritten = false;
  p->mask = 1;
  Impl::Batch b;
  b.target = p->target;
  b.clearStencil = normal ? 0 : 1;
  p->batches.push_back(b);
}
void Backend::beginMaskContent() {
  p->mask = p->maskWritten ? 2 : 0;
  p->reference = 1;
}
void Backend::endMask() { p->mask = 0; }
void Backend::applyMask(const MaskApplyPacket &m) {
  if (p->mask != 1)
    return;
  p->reference = m.isDodge ? 0 : 1;
  if (m.kind == MaskDrawableKind::Part) {
    drawPartPacket(m.partPacket);
    return;
  }
  const auto &mask = m.maskPacket;
  auto v = core::render::sharedVertexBufferData().rawStorage(),
       d = core::render::sharedDeformBufferData().rawStorage();
  auto it = p->indices.find(mask.indexBuffer);
  if (it == p->indices.end() || !mask.vertexCount)
    return;
  Impl::Batch b;
  b.target = p->target;
  b.base = int32_t(p->vertices.size());
  b.first = uint32_t(p->elements.size());
  b.count = uint32_t(it->second.size());
  b.mask = 1;
  b.reference = p->reference;
  b.kind = 8;
  p->maskWritten = true;
  for (uint32_t i = 0; i < mask.vertexCount; ++i) {
    auto clip = transformClip(
        mask.mvp, {v.ptr[mask.vertexOffset + i] + d.ptr[mask.deformOffset + i] -
                       mask.origin.x,
                   v.ptr[mask.vertexOffset + mask.vertexAtlasStride + i] +
                       d.ptr[mask.deformOffset + mask.deformAtlasStride + i] -
                       mask.origin.y,
                   0});
    p->vertices.push_back({clip.x, clip.y, 0, 0});
  }
  p->elements.insert(p->elements.end(), it->second.begin(), it->second.end());
  p->batches.push_back(b);
}
void Backend::beginDynamicComposite(const core::DynamicCompositePass &pass) {
  if (!pass.surface)
    throw std::runtime_error("Dynamic composite missing surface");
  std::array<uint32_t, 3> ids{};
  for (int i = 0; i < 3; ++i)
    if (pass.surface->textures[i])
      ids[i] = pass.surface->textures[i]->backendId();
  if (!ids[0])
    throw std::runtime_error("Dynamic composite missing color target");
  auto &entry = p->composites[pass.surface.get()];
  if (!entry.target || entry.target->colors != ids) {
    if (entry.target)
      p->destroyTarget(*entry.target);
    entry.owner = pass.surface;
    entry.target = p->makeTarget(ids);
  }
  p->stack.push_back(p->target);
  p->target = entry.target.get();
  Impl::Batch b;
  b.target = p->target;
  b.clear = true;
  p->batches.push_back(b);
}
void Backend::endDynamicComposite(const core::DynamicCompositePass &) {
  if (p->stack.empty())
    throw std::runtime_error("Unbalanced dynamic composite");
  Impl::Batch mip; mip.target=p->target; mip.kind=14; p->batches.push_back(mip);
  p->target = p->stack.back();
  p->stack.pop_back();
}
void Backend::destroyDynamicComposite(
    const std::shared_ptr<core::DynamicCompositeSurface> &surface) {
  if (!p)
    return;
  auto it = p->composites.find(surface.get());
  if (it != p->composites.end()) {
    p->c.idle();
    p->destroyTarget(*it->second.target);
    p->composites.erase(it);
  }
}
void Backend::drawSprites(uint32_t texture, const float *data, size_t count,
                          const Mat4 &matrix) {
  if (!count)
    return;
  Impl::Batch b;
  b.target = p->target;
  b.base = int32_t(p->vertices.size());
  b.first = uint32_t(p->elements.size());
  b.count = uint32_t(count);
  b.textures[0] = texture;
  auto &image = p->textures.at(texture).image;
  for (size_t i = 0; i < count; ++i) {
    auto v = matrix.transformPoint({data[i * 8], data[i * 8 + 1], 0});
    p->vertices.push_back({v.x, v.y, data[i * 8 + 2] / image.width,
                           data[i * 8 + 3] / image.height});
    p->elements.push_back(uint16_t(i));
  }
  std::copy_n(data + 4, 4, b.material.tint);
  p->batches.push_back(b);
}

} // namespace nijiexpose::metal
