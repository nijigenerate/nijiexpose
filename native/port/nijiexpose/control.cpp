#include "control.hpp"
#include "json.hpp"
#include "scene/package.hpp"
#include <core/runtime_state.hpp>
#include <core/nodes/drawable.hpp>
#include <fstream>
#include <chrono>
#include <algorithm>
namespace nijiexpose::control {
namespace {
std::filesystem::path inbox;
auto nextPoll=std::chrono::steady_clock::time_point{};
double number(const json::Object& o,const char* key) {
    const auto& v=o.at(key);
    if(!v.is_number())throw std::runtime_error(std::string(key)+" must be numeric");
    double n=v.to_number<double>();
    if(!std::isfinite(n))throw std::runtime_error("Nonfinite value");
    return n;
}
json::Object describe(size_t index,const std::shared_ptr<scene::SceneItem>& item) {
    auto& t=item->puppet->transform;auto b=item->getBounds();
    return {{"index",index},{"path",item->filePath},{"scale",t.scale.x},
        {"position",json::Array{t.translation.x,t.translation.y}},
        {"bounds",json::Array{b[0],b[1],b[2],b[3]}}};
}
json::Value execute(const json::Value& request) {
    struct BoundsScope {
        bool previous=nicxlive::core::nodes::inGetUpdateBounds();
        BoundsScope(){nicxlive::core::nodes::inSetUpdateBounds(true);}
        ~BoundsScope(){nicxlive::core::nodes::inSetUpdateBounds(previous);}
    } boundsScope;
    auto& o=request.as_object();auto command=std::string(o.at("command").as_string());
    auto scene=scene::insScene();int width=0,height=0;nicxlive::core::inGetViewport(width,height);
    if(command=="list") {
        json::Array items;for(size_t i=0;i<scene->sceneItems.size();++i)items.push_back(describe(i,scene->sceneItems[i]));
        return json::Object{{"items",items},{"viewport",json::Array{width,height}}};
    }
    double index=number(o,"index");
    if(index<0||std::floor(index)!=index||index>=scene->sceneItems.size())throw std::runtime_error("Invalid model index");
    auto item=scene->sceneItems[static_cast<size_t>(index)];
    if(command=="parameters") {
        json::Array values;
        for(auto& p:item->puppet->parameters)values.push_back(json::Object{{"uuid",p->uuid},{"name",p->name},{"value",json::Array{p->value.x,p->value.y}},{"defaults",json::Array{p->defaults.x,p->defaults.y}}});
        return values;
    }
    if(item->attachedParent)throw std::runtime_error("Adjust the root model of an attached group");
    auto& t=item->puppet->transform;
    float scale=t.scale.x,x=t.translation.x,y=t.translation.y;
    if(command=="set_transform") {
        if(o.contains("scale"))scale=static_cast<float>(number(o,"scale"));
        if(o.contains("x"))x=static_cast<float>(number(o,"x"));
        if(o.contains("y"))y=static_cast<float>(number(o,"y"));
        if(scale<.25f||scale>5.f||!std::isfinite(x)||!std::isfinite(y))throw std::runtime_error("Scale must be 0.25..5; position must be finite");
    }else if(command=="fit") {
        float margin=o.contains("margin")?static_cast<float>(number(o,"margin")):80;
        item->fitToViewport(margin);
        return describe(static_cast<size_t>(index),item);
    }else throw std::runtime_error("Unknown command");
    t.scale={scale,scale};t.translation.x=x;t.translation.y=y;
    t.update();
    item->updateTransform(); // Keep mouse drag/damping targets in sync with external changes.
    return describe(static_cast<size_t>(index),item);
}
}
void start(const std::filesystem::path& directory) {
    inbox=std::filesystem::absolute(directory);std::filesystem::create_directories(inbox);
}
void poll() {
    if(inbox.empty()||std::chrono::steady_clock::now()<nextPoll)return;
    nextPoll=std::chrono::steady_clock::now()+std::chrono::milliseconds(100);
    std::error_code error;size_t count=0;
    for(auto& entry:std::filesystem::directory_iterator(inbox,error)) {
        auto path=entry.path();auto name=path.filename().string();
        if(!name.ends_with(".request.json")||!entry.is_regular_file())continue;
        if(++count>64)break;
        json::Value response;
        try {
            if(entry.file_size()>16384)throw std::runtime_error("Request exceeds 16 KiB");
            std::ifstream input(path,std::ios::binary);std::string text((std::istreambuf_iterator<char>(input)),{});
            response=json::Object{{"ok",true},{"result",execute(json::parse(text))}};
        }catch(const std::exception& ex){response=json::Object{{"ok",false},{"error",ex.what()}};}
        auto result=inbox/(name.substr(0,name.size()-13)+".response.json");auto temporary=result;temporary+=".tmp";
        std::ofstream out(temporary,std::ios::binary);out<<json::serialize(response);out.close();
        if(out){std::filesystem::rename(temporary,result,error);if(!error)std::filesystem::remove(path,error);}
    }
}
}
