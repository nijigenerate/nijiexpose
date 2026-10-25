// Windows rendering branch of nijilive/fmt/package.d. JSON uses Boost.JSON.
#include "model_io.hpp"
#include "animation/animation.hpp"
#include "automation.hpp"
#include <core/nodes/projectable.hpp>
#include "json.hpp"
#include <fstream>
#include <filesystem>
#include <cstring>
namespace nijiexpose::model_io {
namespace {
thread_local bool isLoadingINP_=false,started=false;
thread_local std::vector<std::shared_ptr<nicxlive::core::Texture>> textureBindings;
constexpr std::string_view magic{"TRNSRTS\0",8},textures="TEX_SECT",extensions="EXT_SECT";
void enforce(bool condition,const char* message) { if(!condition) throw std::runtime_error(message); }
struct Reader {
    std::span<const uint8_t> bytes;
    size_t offset=0;
    std::span<const uint8_t> read(size_t size) {
        if(size>bytes.size()-offset) throw std::out_of_range("INP data range out of bounds");
        auto result=bytes.subspan(offset,size); offset+=size; return result;
    }
    uint32_t u32() { uint32_t result=0; for(auto byte:read(4)) result=(result<<8)|byte; return result; }
    bool section(std::string_view value) {
        auto slice=read(value.size()); return std::memcmp(slice.data(),value.data(),value.size())==0;
    }
};
std::vector<uint8_t> readFile(const std::string& file) {
    std::ifstream stream(std::filesystem::u8path(file),std::ios::binary);
    if(!stream) throw std::runtime_error("Cannot read "+file);
    return {(std::istreambuf_iterator<char>(stream)),std::istreambuf_iterator<char>()};
}
void endTextureLoading(bool checkErrors=true) {
    if(checkErrors) enforce(started,"Texture loading pass not started!");
    started=false; textureBindings.clear();
}
// nicxlive node/parameter implementations consume Boost.PropertyTree Fghj.
// Parse with Boost.JSON once, then retain its exact scalar text in that interface.
nicxlive::core::serde::Fghj toFghj(const json::Value& data) {
    nicxlive::core::serde::Fghj result;
    if(data.is_object()) for(const auto& child:data.as_object()) result.push_back({std::string(child.key()),toFghj(child.value())});
    else if(data.is_array()) for(const auto& child:data.as_array()) result.push_back({"",toFghj(child)});
    else if(data.is_string()) result.put_value(std::string(data.as_string()));
    else result.put_value(json::serialize(data));
    return result;
}
std::shared_ptr<Puppet> loadJson(std::string_view text) {
    auto data=json::parse(text);
    auto puppet=std::make_shared<Puppet>();
    // Resolve Part slot references during nicxlive's finalize call, as the D
    // texture-loading pass makes these same slots available during deserialize.
    if(started) puppet->textureSlots=textureBindings;
    if(auto error=puppet->deserializeFromFghj(toFghj(data))) throw std::runtime_error(*error);
    if(auto values=json::find(data,"automation");values&&!values->is_null()) {
        for(auto& child:values->as_array()) {
            std::string type;
            if(auto value=json::find(child,"type"))json::deserialize(*value,type);
            if(automation::inHasAutomationType(type)) {
                auto entry=automation::inInstantiateAutomation(type,puppet);
                entry->deserializeFromJson(child);
                puppet->automation.push_back(entry);
            }
        }
    }
    if(auto animations=json::find(data,"animations");animations && animations->is_object()) {
        for(const auto& entry:animations->as_object()) {
            auto anim=std::make_shared<animation::Animation>();
            anim->deserializeFromJson(entry.value());
            puppet->animations[std::string(entry.key())]=anim;
        }
    }
    for(auto& entry:puppet->automation) entry->reconstruct(puppet);
    for(auto& [name,anim]:puppet->animations) anim->reconstruct(puppet);
    for(auto& entry:puppet->automation) entry->finalize(puppet);
    for(auto& [name,anim]:puppet->animations) anim->finalize(puppet);
    // Same pre-update initialization as nicxlive::njgLoadPuppet used by nijimi.
    if(auto root=puppet->actualRoot()) {
        auto markProjectablesIgnorePuppet=[&](auto&& self,const std::shared_ptr<nicxlive::core::nodes::Node>& node)->void {
            if(!node)return;
            if(auto projectable=std::dynamic_pointer_cast<nicxlive::core::nodes::Projectable>(node))
                projectable->setIgnorePuppet(true);
            for(const auto& child:node->childrenList())self(self,child);
        };
        markProjectablesIgnorePuppet(markProjectablesIgnorePuppet,root);
        puppet->rescanNodes();
        root->build(true);
    }
    return puppet;
}
void appendU32(std::vector<uint8_t>& out,uint32_t value) {
    for(int shift:{24,16,8,0}) out.push_back(static_cast<uint8_t>(value>>shift));
}
void appendExtensions(std::vector<uint8_t>& out,const std::shared_ptr<Puppet>& puppet) {
    if(puppet->extData.empty()) return;
    out.insert(out.end(),extensions.begin(),extensions.end()); appendU32(out,static_cast<uint32_t>(puppet->extData.size()));
    for(const auto& [name,payload]:puppet->extData) {
        appendU32(out,static_cast<uint32_t>(name.size())); out.insert(out.end(),name.begin(),name.end());
        appendU32(out,static_cast<uint32_t>(payload.size())); out.insert(out.end(),payload.begin(),payload.end());
    }
}
}
bool inIsINPMode() { return isLoadingINP_; }
std::shared_ptr<Puppet> inLoadPuppetFromMemory(std::span<const uint8_t> data) { return loadJson({reinterpret_cast<const char*>(data.data()),data.size()}); }
std::shared_ptr<Puppet> inLoadJSONPuppet(const std::string& data) { isLoadingINP_=false; return loadJson(data); }
std::shared_ptr<Puppet> inLoadPuppet(const std::string& file) {
    try {
        auto buffer=readFile(file); auto ext=std::filesystem::u8path(file).extension().string();
        if(ext==".inp"||ext==".inx") {
            Reader reader{buffer}; enforce(reader.section(magic),ext==".inp" ? "Invalid data format for INP puppet" : "Invalid data format for nijigenerate INX");
            return inLoadINPPuppet(buffer);
        }
        throw std::runtime_error("Invalid file format of "+ext+" at path "+file);
    } catch(const std::exception&) { endTextureLoading(false); throw; }
}
std::shared_ptr<Puppet> inLoadINPPuppet(std::span<const uint8_t> buffer) {
    Reader reader{buffer}; isLoadingINP_=true;
    enforce(reader.section(magic),"Invalid data format for INP puppet");
    auto puppetLength=reader.u32(); auto puppetData=reader.read(puppetLength);
    enforce(reader.section(textures),"Expected Texture Blob section, got nothing!");
    enforce(!started,"Texture loading pass already started!"); started=true;
    auto count=reader.u32();
    std::vector<std::shared_ptr<nicxlive::core::Texture>> slots;
    for(uint32_t i=0;i<count;++i) {
        auto length=reader.u32(); reader.read(1); // textureType is unused in the D loader.
        if(length==0) textureBindings.push_back(std::make_shared<nicxlive::core::Texture>(std::vector<uint8_t>{},0,0,4,4,false,true));
        else {
            auto payload=reader.read(length);
            nicxlive::core::ShallowTexture shallow(std::vector<uint8_t>(payload.begin(),payload.end()));
            textureBindings.push_back(std::make_shared<nicxlive::core::Texture>(shallow));
        }
        slots.push_back(textureBindings.back());
    }
    auto puppet=loadJson({reinterpret_cast<const char*>(puppetData.data()),puppetData.size()});
    puppet->textureSlots=std::move(slots); puppet->updateTextureState(); endTextureLoading();
    if(buffer.size()>=reader.offset+8 && reader.section(extensions)) {
        auto sectionCount=reader.u32();
        for(uint32_t i=0;i<sectionCount;++i) {
            auto nameLength=reader.u32(); auto name=reader.read(nameLength);
            auto payloadLength=reader.u32(); auto payload=reader.read(payloadLength);
            puppet->extData[std::string(name.begin(),name.end())]={payload.begin(),payload.end()};
        }
    }
    return puppet;
}
void inWriteINPExtensions(const std::shared_ptr<Puppet>& puppet,const std::string& file) {
    auto input=readFile(file); Reader reader{input};
    enforce(reader.section(magic),"Invalid data format for INP puppet");
    auto jsonLength=reader.u32(); reader.read(jsonLength);
    enforce(reader.section(textures),"Expected Texture Blob section, got nothing!");
    auto count=reader.u32(); for(uint32_t i=0;i<count;++i) { auto length=reader.u32(); reader.read(size_t(length)+1); }
    size_t extStart=reader.offset,extEnd=reader.offset; bool found=false;
    if(input.size()-reader.offset>=8 && reader.section(extensions)) {
        found=true; auto payloadCount=reader.u32();
        for(uint32_t i=0;i<payloadCount;++i) { auto nameLength=reader.u32(); reader.read(nameLength); auto length=reader.u32(); reader.read(length); }
        extEnd=reader.offset;
    }
    std::vector<uint8_t> output=input;
    if(found) {
        output.assign(input.begin(),input.begin()+extStart); appendExtensions(output,puppet);
        output.insert(output.end(),input.begin()+extEnd,input.end());
    } else appendExtensions(output,puppet);
    std::ofstream stream(std::filesystem::u8path(file),std::ios::binary|std::ios::trunc);
    if(!stream) throw std::runtime_error("Cannot write "+file);
    stream.write(reinterpret_cast<const char*>(output.data()),static_cast<std::streamsize>(output.size()));
    if(!stream) throw std::runtime_error("Cannot complete write to "+file);
}
}
