#pragma once
#include "ft/adaptor.hpp"
#include <chrono>
namespace nijiexpose::tracking {
// Inference's bounded queue is drained on the UI thread. Adaptors read that snapshot.
struct EmbeddedFeed {
    std::map<std::string,float> blends;
    std::map<std::string,ft::Bone> bones;
    bool active=false;
    std::chrono::steady_clock::time_point received{};
};
inline EmbeddedFeed& embeddedFeed(){static thread_local EmbeddedFeed feed;return feed;}
class EmbeddedAdaptor final:public ft::Adaptor {
    bool started=false,receiving=false;
public:
    std::string getAdaptorName() override{return "Embedded nijitrack";}
    void start() override{started=true;}
    void stop() override{started=receiving=false;blendshapes.clear();bones.clear();}
    bool isRunning() override{return started;}
    bool isReceivingData() override{return receiving;}
    std::vector<std::string> getOptionNames() override{return {};}
    void poll() override{
        const auto& feed=embeddedFeed();
        receiving=started && feed.active && std::chrono::steady_clock::now()-feed.received<std::chrono::seconds(1);
        if(receiving){blendshapes=feed.blends;bones=feed.bones;}
        else {blendshapes.clear();bones.clear();}
    }
};
}
