#pragma once
#include <core/render/common.hpp>
#include <core/render/commands.hpp>
#include <core/render/shared_deform_buffer.hpp>
namespace nijiexpose::ogl {
// Direct port of nijilive/core/render/backends/opengl/queue.d.
class RenderQueue final : public nicxlive::core::RenderCommandEmitter {
    nicxlive::core::RenderBackend* activeBackend{};
    nicxlive::core::RenderGpuState* frameState{};
    int dynDepth{};
    bool ready() const { return activeBackend && frameState; }
    void uploadSharedBuffers() {
        using namespace nicxlive::core::render;
        if (!activeBackend) return;
        if (sharedVertexBufferDirty()) {
            const auto& data = sharedVertexBufferData();
            if (data.size()) activeBackend->uploadSharedVertexBuffer(data);
            sharedVertexMarkUploaded();
        }
        if (sharedUvBufferDirty()) {
            const auto& data = sharedUvBufferData();
            if (data.size()) activeBackend->uploadSharedUvBuffer(data);
            sharedUvMarkUploaded();
        }
        if (sharedDeformBufferDirty()) {
            const auto& data = sharedDeformBufferData();
            if (data.size()) activeBackend->uploadSharedDeformBuffer(data);
            sharedDeformMarkUploaded();
        }
    }
public:
    void beginFrame(nicxlive::core::RenderBackend* backend,nicxlive::core::RenderGpuState& state) override {
        activeBackend=backend; frameState=&state;
        state=nicxlive::core::RenderGpuState::init(); dynDepth=0;
        uploadSharedBuffers();
    }
    void endFrame(nicxlive::core::RenderBackend*,nicxlive::core::RenderGpuState&) override {
        activeBackend=nullptr; frameState=nullptr;
    }
    void drawPart(const std::shared_ptr<nicxlive::core::nodes::Part>& part,bool isMask) override {
        if (!ready() || !part) return;
        nicxlive::core::nodes::PartDrawPacket packet;
        part->fillDrawPacket(*part,packet,isMask);
        activeBackend->drawPartPacket(packet);
    }
    void drawPartPacket(const nicxlive::core::nodes::PartDrawPacket& packet) override {
        if (!ready()) return;
        activeBackend->drawPartPacket(packet);
    }
    void beginDynamicComposite(const std::shared_ptr<nicxlive::core::nodes::Projectable>&,const nicxlive::core::DynamicCompositePass& pass) override {
        if (!ready()) return;
        ++dynDepth; activeBackend->beginDynamicComposite(pass);
    }
    void endDynamicComposite(const std::shared_ptr<nicxlive::core::nodes::Projectable>&,const nicxlive::core::DynamicCompositePass& pass) override {
        if (!ready()) return;
        if (dynDepth>0) --dynDepth;
        activeBackend->endDynamicComposite(pass);
    }
    void beginMask(bool stencil) override { if (ready()) activeBackend->beginMask(stencil); }
    void applyMask(const std::shared_ptr<nicxlive::core::nodes::Drawable>& drawable,bool dodge) override {
        if (!ready() || !drawable) return;
        nicxlive::core::RenderBackend::MaskApplyPacket packet;
        if (nicxlive::core::render::tryMakeMaskApplyPacket(drawable,dodge,packet)) activeBackend->applyMask(packet);
    }
    void beginMaskContent() override { if (ready()) activeBackend->beginMaskContent(); }
    void endMask() override { if (ready()) activeBackend->endMask(); }
};
}
