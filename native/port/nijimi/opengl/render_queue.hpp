#pragma once
#include <core/render/backend_queue.hpp>
#include <core/render/command_emitter.hpp>
namespace nijiexpose::ogl {
// Use the same nicxlive command emitter as nijimi. Playback mirrors renderCommands.
class RenderQueue final : public nicxlive::core::render::QueueCommandEmitter {
    std::shared_ptr<nicxlive::core::render::QueueRenderBackend> queue;
    explicit RenderQueue(std::shared_ptr<nicxlive::core::render::QueueRenderBackend> q)
        : QueueCommandEmitter(q), queue(std::move(q)) {}
public:
    RenderQueue() : RenderQueue(std::make_shared<nicxlive::core::render::QueueRenderBackend>()) {}
    void drawPart(const std::shared_ptr<nicxlive::core::nodes::Part>& part,bool isMask) override {
        if(!part)return;
        nicxlive::core::nodes::PartDrawPacket packet;
        part->fillDrawPacket(*part,packet,isMask);
        drawPartPacket(packet);
    }
    void endFrame(nicxlive::core::RenderBackend* backend,nicxlive::core::RenderGpuState& state) override {
        QueueCommandEmitter::endFrame(backend,state);
        using nicxlive::core::render::RenderCommandKind;
        for(const auto& cmd:queue->queue) {
            switch(cmd.kind) {
                case RenderCommandKind::DrawPart: backend->drawPartPacket(cmd.partPacket);break;
                case RenderCommandKind::BeginMask: backend->beginMask(cmd.usesStencil);break;
                case RenderCommandKind::ApplyMask: backend->applyMask(cmd.maskApplyPacket);break;
                case RenderCommandKind::BeginMaskContent: backend->beginMaskContent();break;
                case RenderCommandKind::EndMask: backend->endMask();break;
                case RenderCommandKind::BeginDynamicComposite: backend->beginDynamicComposite(cmd.dynamicPass);break;
                case RenderCommandKind::EndDynamicComposite: backend->endDynamicComposite(cmd.dynamicPass);break;
                default:break;
            }
        }
    }
};
}

