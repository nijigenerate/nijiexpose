#include "nijiui/core/window/appwin.hpp"
#include "nijiui/core/app.hpp"
#include "nijiui/core/core.hpp"
#include "panels/scene.hpp"
#include "panels/plugin.hpp"
#include "scene/package.hpp"
#include "plugins/package.hpp"
#include "nijilive/model_io.hpp"
#include "nijilive/opengl/backend.hpp"
#include <core/runtime_state.hpp>
#include "capture_frame.hpp"
#include <iostream>
namespace core=nicxlive::core;
using namespace nijiexpose;
class VisualWindow:public nijiui::InApplicationWindow {
public:
    std::shared_ptr<ogl::Backend> backend;
    panels::ScenePanel settings;
    panels::PluginPanel pluginPanel;
    size_t redPixels=0;
    VisualWindow(const char* model):InApplicationWindow("nijiexpose Windows visual verification",1000,660,0){
        backend=std::make_shared<ogl::Backend>();core::inSetRenderBackend(backend);backend->initializeRenderer();core::inSetViewport(256,256);core::inSetClearColor(0,0,0,0);
        auto scene=scene::insScene();scene->space=std::make_shared<tracking::VirtualSpace>();scene->shouldPostProcess=false;scene->addPuppet(model,model_io::inLoadPuppet(model));scene->sceneItems[0]->fitOnFirstDraw=false; /* Explicit pixel-test scale below. */ auto puppet=scene->sceneItems[0]->puppet;for(auto& texture:puppet->textureSlots)texture->setFiltering(core::Filtering::Nearest);auto& transform=puppet->transform;transform.scale={16,16};transform.update();settings.init_();pluginPanel.init_();
    }
    void onUpdate()override{
        scene::insScene()->update();std::vector<uint8_t> pixels(256*256*4);backend->dumpViewport(pixels,256,256);redPixels=0;for(size_t i=0;i<pixels.size();i+=4)if(pixels[i]==255&&pixels[i+1]==0&&pixels[i+2]==0&&pixels[i+3]==255)++redPixels;
        ogl::glBindFramebuffer(GL_FRAMEBUFFER,0);
        ImGui::SetNextWindowPos({12,12},ImGuiCond_Always);ImGui::SetNextWindowSize({460,636},ImGuiCond_Always);ImGui::Begin("Windows GPU test / actual rendered frame",nullptr,ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize);
        ImGui::TextUnformatted("SDL3 + ImGui 1.93 + nicxlive");ImGui::Separator();
        ImGui::TextColored({.3f,1,.5f,1},"%s  Original D fixture loaded",scene::insScene()->sceneItems.size()==1?"PASS":"FAIL");
        ImGui::TextColored(redPixels==16384?ImVec4{.3f,1,.5f,1}:ImVec4{1,.3f,.3f,1},"%s  GPU pixel check: %zu / 16384",redPixels==16384?"PASS":"FAIL",redPixels);
        ImGui::TextUnformatted("Fixture: 8 x 8 red mesh, scale 16, nearest");
        ImGui::Image(ImTextureRef(static_cast<ImTextureID>(backend->mainAlbedoHandle())),{256,256},{0,1},{1,0});
        ImGui::Separator();ImGui::TextUnformatted("Real Scene Settings panel is shown at right.");ImGui::TextUnformatted("Japanese font: 日本語表示の確認");ImGui::TextWrapped("This is a component verification window. Full application migration is still in progress.");ImGui::End();
        ImGui::SetNextWindowPos({484,12},ImGuiCond_Always);ImGui::SetNextWindowSize({504,480},ImGuiCond_Always);ImGui::Begin("Scene Settings - migrated implementation",nullptr,ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize);settings.renderSceneSettingsContent();ImGui::End();
        ImGui::SetNextWindowPos({484,504},ImGuiCond_Always);ImGui::SetNextWindowSize({504,144},ImGuiCond_Always);ImGui::Begin("Plugin panel",nullptr,ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize);pluginPanel.updateEmbedded();ImGui::End();
    }
};
int main(int argc,char** argv){try{
    if(argc!=4)throw std::runtime_error("Expected model, plugin directory, screenshot path");
    nijiui::inSetApplication({"nijiexpose-visual-test","nijiexpose-visual-test","Windows visual test"});nijiui::inInitUI();
    auto lua=luaL_newstate();lua_pushvalue(lua,LUA_GLOBALSINDEX);lua_setfield(lua,LUA_GLOBALSINDEX,"_G");plugins::insPluginRegisterAll(lua);plugins::insPlugins.push_back(std::make_shared<plugins::Plugin>(plugins::PluginInfo{"visual","Plugin UI fixture","1"},argv[2],lua,true));
    {VisualWindow window(argv[1]);for(int i=0;i<4;++i)window.update();captureFrontBuffer(argv[3],window.width(),window.height());std::cerr<<"red pixels="<<window.redPixels<<'\n';if(window.redPixels!=16384)throw std::runtime_error("GPU fixture coverage mismatch");scene::insScene()->sceneItems.clear();plugins::insPlugins.clear();lua_close(lua);window.close();}
    SDL_Quit();std::cout<<"Visual test passed; actual framebuffer saved\n";return 0;
}catch(const std::exception& ex){std::cerr<<ex.what()<<'\n';return 1;}}
