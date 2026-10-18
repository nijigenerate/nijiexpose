#pragma once
#include "nijiexpose/animation/package.hpp"
namespace nijiexpose::scene {
using nicxlive::core::Puppet;
using nicxlive::core::nodes::Node;
using nicxlive::core::math::Vec2;
class SceneItem:public std::enable_shared_from_this<SceneItem> {
public:
    std::string filePath;
    std::shared_ptr<Puppet> puppet;
    std::shared_ptr<Node> puppetRoot;
    std::shared_ptr<SceneItem> attachedParent;
    std::vector<std::shared_ptr<tracking::TrackingBinding>> bindings;
    std::vector<std::shared_ptr<animation::AnimationControl>> animations;
    std::unique_ptr<animation::AnimationPlayer> player;
    Vec2 startPos{NAN,NAN},targetPos{0,0},targetSize{0,0};
    float targetScale=0,zSort=0;
    bool fitOnFirstDraw=false;
    std::vector<std::shared_ptr<SceneItem>> children;
    std::string name() const;
    void saveBindings();
    void saveAnimations();
    bool tryLoadBindings();
    bool tryLoadAnimations();
    void genBindings();
    void genAnimationControls();
    void sleep();
    void awake();
    std::array<float,4> getBounds();
    bool attachTo(const std::shared_ptr<SceneItem>& parent);
    bool detach();
    bool intersects(Vec2 mousePos);
    void draw();
    void updateTransform();
    void fitToViewport(float margin=80, bool enlarge=true);
    std::string dump(int index=0) const;
};
}
