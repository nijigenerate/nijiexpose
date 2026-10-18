// Port of nijiui/widgets/dialog.d, Windows InBranding configuration.
#include "dialog.hpp"
#include "basic.hpp"
#include "input.hpp"
#include "image.hpp"
#include "nijiui/core/font.hpp"
#include "nijiui/core/resource.hpp"
#include "nijilive/texture_utils.hpp"
#include "i18n/tr.hpp"
#include <texture.hpp>
#include <array>
#include <memory>
#include <vector>
namespace nijiui {
namespace {
thread_local std::array<std::shared_ptr<nicxlive::core::Texture>,3> adaTextures;
struct DialogEntry {
    const char* tag;
    const char* title;
    std::string text;
    DialogLevel level;
    DialogButtons btns,selected;
    int btncount;
    void* userData;
};
thread_local std::vector<DialogEntry> entries;
[[maybe_unused]] DialogEntry* findDialogEntry(const char* tag) {
    for(auto& entry:entries) if(entry.tag==tag) return &entry;
    return nullptr;
}
}
void uiImInitDialogs() {
    auto load=[](int id) {
        auto resource=embeddedResource(id);
        nicxlive::core::ShallowTexture texture(std::vector<uint8_t>(resource.begin(),resource.end()));
        nijiexpose::inTexPremultiply(texture.data);
        return texture;
    };
    auto infoTex=load(4201);
    auto warnTex=load(4202);
    auto errTex=load(4203);
    adaTextures={std::make_shared<nicxlive::core::Texture>(infoTex),std::make_shared<nicxlive::core::Texture>(warnTex),std::make_shared<nicxlive::core::Texture>(errTex)};
}
void uiImRenderDialogs() {
    if(entries.empty()) return;
    auto* entry=&entries[0];
    if(!ImGui::IsPopupOpen(entry->title)) ImGui::OpenPopup(entry->title);
    auto flags=ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_AlwaysAutoResize;
    if(ImGui::BeginPopupModal(entry->title,nullptr,flags)) {
        float uiScale=inGetUIScale(),errImgScale=112*uiScale,msgEndPadding=4*uiScale;
        ImGui::BeginGroup();
        if(ImGui::BeginChild("ErrorMainBoxLogo",{errImgScale,errImgScale}))
            uiImImage(adaTextures.at(static_cast<size_t>(entry->level))->backendId(),{errImgScale,errImgScale});
        ImGui::EndChild();
        ImGui::SameLine(0,0);
        ImGui::PushTextWrapPos(512*uiScale);
        uiImLabel(entry->text);
        ImGui::PopTextWrapPos();
        ImGui::SameLine(0,0);
        uiImDummy({msgEndPadding,1});
        ImGui::EndGroup();
        auto avail=uiImAvailableSpace();
        float btnHeight=24*uiScale,btnSize=80*uiScale,totalBtnSize=btnSize*entry->btncount;
        float msgAreaWidth=errImgScale+uiImMeasureString(entry->text).x+msgEndPadding;
        float requestedMinimumSize=256*uiScale;
        if(msgAreaWidth<requestedMinimumSize && totalBtnSize<requestedMinimumSize) {
            ImGui::Dummy({requestedMinimumSize-(totalBtnSize+1),btnHeight});
            ImGui::SameLine(0,0);
        } else if(avail.x>totalBtnSize) {
            ImGui::Dummy({avail.x-(totalBtnSize+1),btnHeight});
            ImGui::SameLine(0,0);
        }
        if((int(entry->btns)&2)==2) {
            if(uiImButton(i18n::__("Cancel"),{btnSize,btnHeight})) { entry->selected=DialogButtons::Cancel; ImGui::CloseCurrentPopup(); }
            ImGui::SameLine(0,0);
        }
        if((int(entry->btns)&1)==1) {
            if(uiImButton(i18n::__("OK"),{btnSize,btnHeight})) { entry->selected=DialogButtons::OK; ImGui::CloseCurrentPopup(); }
            ImGui::SameLine(0,0);
        }
        if((int(entry->btns)&8)==8) {
            if(uiImButton(i18n::__("No"),{btnSize,btnHeight})) { entry->selected=DialogButtons::No; ImGui::CloseCurrentPopup(); }
            ImGui::SameLine(0,0);
        }
        if((int(entry->btns)&4)==4) {
            if(uiImButton(i18n::__("Yes"),{btnSize,btnHeight})) { entry->selected=DialogButtons::Yes; ImGui::CloseCurrentPopup(); }
        }
        ImGui::EndPopup();
    }
}
void uiImCleanupDialogs() { if(!entries.empty() && int(entries[0].selected)>0) entries.erase(entries.begin()); }
void uiImDialog(const char* title,std::string body,DialogLevel level,DialogButtons btns,void* data) { uiImDialog(title,title,std::move(body),level,btns,data); }
void uiImDialog(const char* tag,const char* title,std::string body,DialogLevel level,DialogButtons btns,void* data) {
    int count=0;
    if((int(btns)&1)==1) ++count;
    if((int(btns)&2)==2) ++count;
    if((int(btns)&4)==4) ++count;
    if((int(btns)&8)==8) ++count;
    entries.push_back({tag,title,std::move(body),level,btns,DialogButtons::NONE,count,data});
}
DialogButtons uiImDialogButtonSelected(const char* tag) {
    if(entries.empty()) return DialogButtons::NONE;
    if(entries[0].tag!=tag) return DialogButtons::NONE;
    return entries[0].selected;
}
void* uiImDialogButtonUserData(const char* tag) {
    if(entries.empty()) return nullptr;
    if(entries[0].tag!=tag) return nullptr;
    return entries[0].userData;
}
}
