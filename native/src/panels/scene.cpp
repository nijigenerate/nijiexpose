#include "render/settings.hpp"
#include "scene.hpp"
#include "scene/package.hpp"
#include "io/package.hpp"
#include "nijilive/texture_utils.hpp"
#include "nijilive/opengl/runtime.hpp"
#include "nijiui/widgets/input.hpp"
#include "nijiui/widgets/dialog.hpp"
#include "nijiui/core/settings.hpp"
#include "i18n/tr.hpp"
#include <core/runtime_state.hpp>
#include <filesystem>
namespace nijiexpose::panels {
using namespace nijiui;using i18n::_;using i18n::__;
ScenePanel::ScenePanel():Panel("Scene Settings",_("Scene Settings"),true){}
void ScenePanel::loadBackground(const std::string& file){auto scene=scene::insScene();if(file.empty()){scene->bgPath.clear();scene->backgroundImage.reset();inSettingsSetJson("bgPath",nullptr);return;}
    try{scene->bgPath=file;nicxlive::core::ShallowTexture tex(file);if(tex.channels==4)inTexPremultiply(tex.data);scene->backgroundImage=std::make_shared<nicxlive::core::Texture>(tex);inSettingsSet("bgPath",scene->bgPath);}
    catch(const std::exception& ex){auto text=_("Could not load %s, %s");for(auto arg:{file,std::string(ex.what())}){auto pos=text.find("%s");if(pos!=std::string::npos)text.replace(pos,2,arg);}uiImDialog(__("Error"),text);}}
void ScenePanel::setBGColor(nicxlive::core::math::Vec4 color){clearColor=color;nicxlive::core::inSetClearColor(color.x,color.y,color.z,color.w);inSettingsSet("bgColor",std::array<float,4>{color.x,color.y,color.z,color.w});}
void ScenePanel::renderSceneSettingsContent(){auto scene=scene::insScene();uiImLabelColored(_("Post Processing"),{.8f,.3f,.3f,1});uiImSeperator();uiImIndent();if(uiImCheckbox(__("Enabled###POST_PROCESSING"),scene->shouldPostProcess))inSettingsSet("shouldPostProcess",scene->shouldPostProcess);uiImUnindent();
    uiImLabelColored(_("Lighting"),{.8f,.3f,.3f,1});uiImSeperator();uiImIndent();float light[3]{render::inSceneAmbientLight.x,render::inSceneAmbientLight.y,render::inSceneAmbientLight.z};if(uiImColorButton3("###LIGHT_COLOR",&light)){render::inSceneAmbientLight={light[0],light[1],light[2]};inSettingsSet("ambientLight",std::array<float,3>{light[0],light[1],light[2]});}uiImUnindent();
    uiImLabelColored(_("Background Color"),{.8f,.3f,.3f,1});uiImSeperator();uiImIndent();float color[4]{clearColor.x,clearColor.y,clearColor.z,clearColor.w};if(uiImColorButton4("###BG_COLOR",&color))setBGColor({color[0],color[1],color[2],color[3]});
    uiImLabelColored(_("Color Swatches"),{.8f,.3f,.3f,1});uiImIndent();if(uiImColorSwatch4(__("Full Transparent"),{0,0,0,0}))setBGColor({0,0,0,0});uiImSameLine(0,4);if(uiImColorSwatch4(__("Chroma Key Red"),{1,0,0,1}))setBGColor({1,0,0,1});uiImSameLine(0,4);if(uiImColorSwatch4(__("Chroma Key Green"),{0,1,0,1}))setBGColor({0,1,0,1});uiImSameLine(0,4);if(uiImColorSwatch4(__("Chroma Key Blue"),{0,0,1,1}))setBGColor({0,0,1,1});uiImUnindent();uiImUnindent();
    uiImLabelColored(_("Background Image"),{.8f,.3f,.3f,1});uiImSeperator();if(uiImButton(__("Select Background"))){std::array<io::TFD_Filter,3> filters{{{{"*.png"},"PNG File (*.png)"},{{"*.jpg","*.jpeg"},"JPEG (*.jpeg, *.jpeg)"},{{"*.tga"},"TARGA (*.tga)"}}};auto file=io::insShowOpenDialog(filters,_("Open..."));if(!file.empty())loadBackground(file);}
    if(ImGui::BeginDragDropTarget()){if(auto payload=ImGui::AcceptDragDropPayload("_FILEDROP")){auto files=*static_cast<const std::vector<std::string>* const*>(payload->Data);if(!files->empty()){auto file=files->front();auto ext=std::filesystem::u8path(file).extension().string();for(char& c:ext)if(c>='A'&&c<='Z')c+=32;if(ext==".png"||ext==".tga"||ext==".jpeg"||ext==".jpg")loadBackground(file);else{auto text=_("Could not load %s, unsupported file format.");auto pos=text.find("%s");if(pos!=std::string::npos)text.replace(pos,2,file);uiImDialog(__("Error"),text);}}}}
    // Original does not call EndDragDropTarget here; preserve the call sequence.
    if(scene->backgroundImage){ImGui::SameLine(0,4);if(uiImButton(__("\ue5cd")))loadBackground("");}
}
void ScenePanel::onUpdate(){renderSceneSettingsContent();}
}
