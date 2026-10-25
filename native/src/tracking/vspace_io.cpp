// JSON and settings portion of the same original vspace.d implementation.
#include "vspace.hpp"
#include "ext/package.hpp"
#include "log.hpp"
#include "nijiui/core/settings.hpp"
namespace nijiexpose::tracking {
json::Value VirtualSpaceZone::serialize() const {
    json::Array values;
    for(auto& source:sources) {
        if(!source) continue;
        json::Object value{{"type",source->getAdaptorName()}};
        if(!source->getOptions().empty()) {
            json::Object options;
            for(auto& [key,text]:source->getOptions()) options[key]=text;
            value["options"]=std::move(options);
        }
        values.push_back(std::move(value));
    }
    return json::Object{{"name",name},{"sources",std::move(values)}};
}
void VirtualSpaceZone::deserializeFromJson(const json::Value& data) {
    if(auto value=json::find(data,"name")) json::deserialize(*value,name);
    auto values=json::find(data,"sources"); if(!values || values->is_null()) return;
    for(auto& child:values->as_array()) {
        std::string type; if(auto value=json::find(child,"type")) json::deserialize(*value,type);
        try {
            std::map<std::string,std::string> options;
            auto value=json::find(child,"options");
            bool hasOptions=value && !value->is_null();
            if(hasOptions) for(auto& item:value->as_object()) {
                std::string text; json::deserialize(item.value(),text); options[std::string(item.key())]=text;
            }
            options["appName"]="nijiexpose";
            if(type=="VMC Receiver" || type=="Phiz OSC Receiver") options["address"]="0.0.0.0";
            auto adaptor=neCreateAdaptor(type); if(!adaptor) continue;
            adaptor->setOptions(std::move(options)); sources.push_back(adaptor);
            if(hasOptions) adaptor->start();
        } catch(const std::exception& ex) { insLogErr("%s: %s",name,ex.what()); }
    }
}
json::Value VirtualSpace::serialize() const {
    insLogInfo("Saving Virtual Space...");
    json::Array values;
    for(auto& zone:zones) { insLogInfo("Saving Zone %s...",zone->name); values.push_back(zone->serialize()); }
    return json::Object{{"zones",std::move(values)}};
}
void VirtualSpace::deserializeFromJson(const json::Value& data) {
    if(auto values=json::find(data,"zones");values && !values->is_null()) for(auto& child:values->as_array()) {
        auto zone=std::make_shared<VirtualSpaceZone>(); zone->deserializeFromJson(child); zones.push_back(zone);
    }
    rebuildZoneList();
}
void insSaveVSpace(const std::shared_ptr<VirtualSpace>& space) {
    nijiui::inSettingsSetJson("space",space?space->serialize():json::Value{}); nijiui::inSettingsSave();
}
std::shared_ptr<VirtualSpace> insLoadVSpace() {
    auto space=std::make_shared<VirtualSpace>();
    if(!nijiui::inSettingsCanGet("space")) { insLogInfo("No VirtualSpace found, creating a new one..."); return space; }
    space->deserializeFromJson(nijiui::inSettingsGetJson("space")); return space;
}
}
