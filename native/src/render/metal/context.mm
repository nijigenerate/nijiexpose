// Adapted from nijigenerate. BSD-2-Clause; see LICENSE.
#include "context_internal.hpp"
#include "backend.hpp"
#include "render/window_mode.hpp"
#include "metal_shaders.hpp"
#include <SDL3/SDL_metal.h>
#include <imgui_impl_metal.h>
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <cstdlib>
extern "C" void* objc_autoreleasePoolPush(void);
extern "C" void objc_autoreleasePoolPop(void*);
namespace nijiexpose::metal {
namespace { thread_local Context* current{}; }
Context& context() {
    if(!current) throw std::logic_error("Metal context is not active");
    return *current;
}
Context::Context(SDL_Window* window) : p(std::make_unique<Impl>()) {
    if(current) throw std::logic_error("Only one application Metal context is supported");
    @autoreleasepool {
        p->window=window;
        p->device=MTLCreateSystemDefaultDevice();
        if(!p->device) throw std::runtime_error("No Metal device is available");
        // Framebuffer fetch and ordered attachment access are Apple GPU features.
        advancedBlendSupported=advancedBlendCoherent=[p->device supportsFamily:MTLGPUFamilyApple4];
        p->queue=[p->device newCommandQueue];
        if(!p->queue) throw std::runtime_error("Could not create Metal command queue");
        if (window) {
            p->view=SDL_Metal_CreateView(window);
            if(!p->view) throw std::runtime_error(SDL_GetError());
            p->layer=(__bridge CAMetalLayer*)SDL_Metal_GetLayer(p->view);
        }
        p->layer.device=p->device;
        p->layer.pixelFormat=MTLPixelFormatBGRA8Unorm;
        p->layer.framebufferOnly=YES;
        p->layer.opaque=YES;
        p->layer.displaySyncEnabled=YES;

        NSError* error=nil;
        MTLCompileOptions* options=[MTLCompileOptions new];
        options.languageVersion=MTLLanguageVersion2_3;
        options.fastMathEnabled=NO;
        options.preprocessorMacros=@{@"NG_FRAMEBUFFER_FETCH":@(advancedBlendSupported)};
        p->library=[p->device newLibraryWithSource:[NSString stringWithUTF8String:render_source] options:options error:&error];
        if(!p->library) throw std::runtime_error(error.localizedDescription.UTF8String);
        auto pipeline=[MTLRenderPipelineDescriptor new];
        pipeline.vertexFunction=[p->library newFunctionWithName:@"presentVertex"];
        pipeline.fragmentFunction=[p->library newFunctionWithName:@"presentFragment"];
        pipeline.colorAttachments[0].pixelFormat=MTLPixelFormatBGRA8Unorm;
        p->presentPipeline=[p->device newRenderPipelineStateWithDescriptor:pipeline error:&error];
        if(!p->presentPipeline) throw std::runtime_error(error.localizedDescription.UTF8String);
        resize();
        current=this;
    }
}
Context::~Context() {
    try { idle(); } catch(const std::exception& e) { SDL_LogError(SDL_LOG_CATEGORY_RENDER,"%s",e.what()); }
    if(p->backend) p->backend->shutdown();
    shutdownImGui();
    auto pool=p->framePool;
    p.reset();
    current=nullptr;
    if(pool) objc_autoreleasePoolPop(pool);
}
void Context::idle() {
    if(p->last) {
        auto submitted=p->last;
        p->last=nil;
        [submitted waitUntilCompleted];
        if(submitted.status==MTLCommandBufferStatusError) p->pointerSamplePending=false;
        checkCommand(submitted);
        if(p->profile && submitted.GPUEndTime>=submitted.GPUStartTime) {
            p->gpuSeconds+=submitted.GPUEndTime-submitted.GPUStartTime;
            if(++p->profileFrames==120) {
                std::fprintf(stderr,"[metal-gpu] frame_gpu_ms=%.3f (includes UI)\n",p->gpuSeconds*1000/120);
                p->profileFrames=0; p->gpuSeconds=0;
            }
        }
    }
}
void Context::resize() {
    // Offline processing uses the same textures and compute queue without a drawable.
    if (!p->window) return;
    SDL_GetWindowSizeInPixels(p->window,&p->width,&p->height);
    const auto size=CGSizeMake(p->width,p->height);
    if(p->width>0 && p->height>0 && !CGSizeEqualToSize(p->layer.drawableSize,size)) p->layer.drawableSize=size;
}
void Context::setThrottle(int rate) { p->layer.displaySyncEnabled=rate>0; }
void Context::initializeImGui() {
    p->imgui=ImGui_ImplMetal_Init(p->device);
    if(!p->imgui) throw std::runtime_error("ImGui Metal initialization failed");
}
void Context::shutdownImGui() {
    if(!p || !p->imgui) return;
    try { idle(); } catch(const std::exception& e) { SDL_LogError(SDL_LOG_CATEGORY_RENDER,"%s",e.what()); }
    ImGui_ImplMetal_Shutdown();
    p->imgui=false;
}
bool Context::beginFrame() {
    idle();
    if(p->framePool) objc_autoreleasePoolPop(p->framePool);
    p->framePool=objc_autoreleasePoolPush();
    resize();
    const bool transparent=render::transparentWindow;
    const bool sampled=transparent && p->pointerSamplePending;
    const auto* pixel=sampled ? static_cast<const uint8_t*>(p->pointerPixel.contents) : nullptr;
    render::updateWindowMousePassthrough(p->window, sampled && pixel[3]<8, !transparent || sampled);
    p->pointerSamplePending=false;
    if(p->layer.framebufferOnly==transparent) p->layer.framebufferOnly=!transparent;
    if(p->width<=0 || p->height<=0) return false;
    p->drawable=[p->layer nextDrawable];
    if(!p->drawable) return false;
    p->command=[p->queue commandBuffer];
    if(!p->command) throw std::runtime_error("Could not create Metal command buffer");
    p->command.label=@"nijiexpose Metal frame";
    p->uiPass=[MTLRenderPassDescriptor renderPassDescriptor];
    p->uiPass.colorAttachments[0].texture=p->drawable.texture;
    ImGui_ImplMetal_NewFrame(p->uiPass);
    return true;
}
void Context::present(ImDrawData* data, const Image& scene) {
    auto pass=p->uiPass;
    pass.colorAttachments[0].loadAction=MTLLoadActionDontCare;
    pass.colorAttachments[0].storeAction=MTLStoreActionStore;
    auto encoder=[p->command renderCommandEncoderWithDescriptor:pass];
    encoder.label=@"Scene and UI presentation";
    const uint32_t transparent=render::transparentWindow;
    [encoder setRenderPipelineState:p->presentPipeline];
    [encoder setFragmentTexture:native(scene) atIndex:0];
    [encoder setFragmentBytes:&transparent length:sizeof(transparent) atIndex:0];
    [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
    ImGui_ImplMetal_RenderDrawData(data,p->command,encoder);
    [encoder endEncoding];
    if(transparent) {
        int x=0,y=0;
        if(render::windowPointerPixel(p->window,p->width,p->height,x,y)) {
            if(!p->pointerPixel)
                p->pointerPixel=[p->device newBufferWithLength:256 options:MTLResourceStorageModeShared];
            if(!p->pointerPixel) throw std::runtime_error("Could not allocate Metal pointer sample");
            auto blit=[p->command blitCommandEncoder];
            [blit copyFromTexture:p->drawable.texture sourceSlice:0 sourceLevel:0
                sourceOrigin:MTLOriginMake(x,y,0) sourceSize:MTLSizeMake(1,1,1)
                toBuffer:p->pointerPixel destinationOffset:0 destinationBytesPerRow:256
                destinationBytesPerImage:256];
            [blit endEncoding];
            p->pointerSamplePending=true;
        } else {
            render::updateWindowMousePassthrough(p->window,true);
        }
    }
    [p->command presentDrawable:p->drawable];
    [p->command commit];
    p->last=p->command;
    p->command=nil;
    p->drawable=nil;
    p->uiPass=nil;
}
bool Context::recording() const { return p->command!=nil; }
void Context::immediate(const std::function<void()>& fn) {
    if(recording()) { fn(); return; }
    @autoreleasepool {
        idle();
        p->command=[p->queue commandBuffer];
        try { fn(); } catch(...) { p->command=nil; throw; }
        [p->command commit]; [p->command waitUntilCompleted];
        auto done=p->command; p->command=nil; checkCommand(done);
    }
}
Image Context::createImage(uint32_t w,uint32_t h,uint32_t levels,bool depth,bool memoryless) {
    @autoreleasepool {
        if(!w || !h || w>uint32_t(maxTextureSize()) || h>uint32_t(maxTextureSize()))
            throw std::invalid_argument("Invalid Metal texture dimensions");
        auto d=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:depth?MTLPixelFormatDepth32Float_Stencil8:MTLPixelFormatRGBA8Unorm width:w height:h mipmapped:levels>1];
        d.mipmapLevelCount=levels;
        d.storageMode=memoryless?MTLStorageModeMemoryless:MTLStorageModePrivate;
        d.usage=MTLTextureUsageRenderTarget | (depth?0:MTLTextureUsageShaderRead);
        return {retainMetal([p->device newTextureWithDescriptor:d]),w,h,levels};
    }
}
void Context::destroy(Image& image) { image={}; }
void Context::upload(Image& image,const std::vector<uint8_t>& bytes) {
    if(bytes.size()!=size_t(image.width)*image.height*4) throw std::invalid_argument("Metal texture upload size mismatch");
    immediate([&] {
        const auto rowBytes=(size_t(image.width)*4+255)&~size_t(255);
        auto staging=[p->device newBufferWithLength:rowBytes*image.height options:MTLResourceStorageModeShared];
        if(!staging) throw std::runtime_error("Metal texture staging allocation failed");
        for(uint32_t y=0;y<image.height;++y)
            std::memcpy(static_cast<uint8_t*>(staging.contents)+y*rowBytes,bytes.data()+size_t(y)*image.width*4,size_t(image.width)*4);
        auto blit=[p->command blitCommandEncoder];
        [blit copyFromBuffer:staging sourceOffset:0 sourceBytesPerRow:rowBytes sourceBytesPerImage:rowBytes*image.height sourceSize:MTLSizeMake(image.width,image.height,1) toTexture:native(image) destinationSlice:0 destinationLevel:0 destinationOrigin:MTLOriginMake(0,0,0)];
        if(image.mipLevels>1) [blit generateMipmapsForTexture:native(image)];
        [blit endEncoding];
    });
}
void Context::generateMipmaps(Image& image) {
    if(image.mipLevels<2) return;
    immediate([&] { auto blit=[p->command blitCommandEncoder]; [blit generateMipmapsForTexture:native(image)]; [blit endEncoding]; });
}
std::vector<uint8_t> Context::read(Image& image) {
    @autoreleasepool {
        if(recording()) throw std::logic_error("Readback must occur outside frame submission");
        idle();
        const auto rowBytes=(size_t(image.width)*4+255)&~size_t(255);
        auto buffer=[p->device newBufferWithLength:rowBytes*image.height options:MTLResourceStorageModeShared];
        if(!buffer) throw std::runtime_error("Metal readback allocation failed");
        auto command=[p->queue commandBuffer];
        auto blit=[command blitCommandEncoder];
        [blit copyFromTexture:native(image) sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0,0,0) sourceSize:MTLSizeMake(image.width,image.height,1) toBuffer:buffer destinationOffset:0 destinationBytesPerRow:rowBytes destinationBytesPerImage:rowBytes*image.height];
        [blit endEncoding]; [command commit]; [command waitUntilCompleted]; checkCommand(command);
        std::vector<uint8_t> bytes(size_t(image.width)*image.height*4);
        for(uint32_t y=0;y<image.height;++y)
            std::memcpy(bytes.data()+size_t(y)*image.width*4,static_cast<const uint8_t*>(buffer.contents)+y*rowBytes,size_t(image.width)*4);
        return bytes;
    }
}
ImTextureID Context::texture(Image& image) { return ImTextureID(reinterpret_cast<uintptr_t>(image.owner.get())); }
int Context::width() const { return p->width; }
int Context::height() const { return p->height; }
int Context::maxTextureSize() const { return 16384; }
}
