#include "nijiui/core/window/appwin.hpp"
#include "nijiui/core/app.hpp"
#include "nijiui/core/core.hpp"
#include "nijiui/core/font.hpp"
#include "nijiui/widgets/dialog.hpp"
#include "nijilive/opengl/backend.hpp"
#include <core/runtime_state.hpp>
#include <imgui_internal.h>
#include <iostream>
#include <stdexcept>
#include "capture_frame.hpp"
namespace ui=nijiui;
void require(bool value,const char* message) { if(!value) throw std::runtime_error(message); }
class Window final:public ui::InApplicationWindow {
public:
    std::vector<char> order;
    std::vector<std::string> files;
    int closes=0,resizes=0;
    ImVec2 okButton{};
    Window():InApplicationWindow("nijiexpose UI verification",640,480,0) {}
    void onUpdate() override { order.push_back('U'); files=getDraggedFiles(); }
    void onEarlyUpdate() override {
        order.push_back('E');
        auto* modal=ImGui::FindWindowByName("Dialog verification");
        if(modal) okButton={modal->Pos.x+modal->Size.x-45,modal->Pos.y+modal->Size.y-20};
    }
    void onClosed() override { ++closes; }
    void onResized(int w,int h) override { ++resizes; InApplicationWindow::onResized(w,h); }
};
class Child final:public ui::InWindow {
    std::vector<char>& order;
public:
    explicit Child(std::vector<char>& value):order(value) {}
    void onUpdate() override { order.push_back('u'); }
    void onEarlyUpdate() override { order.push_back('e'); }
    void focus() override {} void close() override {} bool isAlive() override { return true; }
    int width() override { return 1; } int height() override { return 1; }
};
int main(int argc,char** argv) {
    try {
        ui::inSetApplication({"nijiexpose-ui-test","nijiexpose-ui-test","UI verification"});
        ui::inInitUI();
        auto backend=std::make_shared<nijiexpose::ogl::Backend>();
        nicxlive::core::inSetRenderBackend(backend);
        {
            Window window;
            backend->initializeRenderer();
            ui::uiImInitDialogs();
            auto child=std::make_shared<Child>(window.order);
            ui::inWindowListAdd(child);
            window.update();
            require(window.order==std::vector<char>{'U','e','u','E'},"Window callback order differs from D");
            require(window.shouldProcess() && window.isAlive(),"Window state invalid");
            require(window.width()==640 && window.height()==480,"Window size differs");
            require(ImGui::GetIO().Fonts->Fonts.Size>0,"Embedded fonts missing");
            ui::inSetUIScale(1.5f);
            require(ui::inGetUIScaleText()=="150%" && ImGui::GetStyle().FontScaleMain==.75f,"UI scale differs");
            ui::inFontsBeginLarge();
            require(ImGui::GetStyle().FontScaleMain==1.5f,"Large font scale differs");
            ui::inFontsEndLarge();
            ui::inSetUIScale(1);
            SDL_Event drop{}; drop.type=SDL_EVENT_DROP_FILE; drop.drop.data="test-image.png";
            require(SDL_PushEvent(&drop),SDL_GetError());
            window.update();
            require(window.files==std::vector<std::string>{"test-image.png"},"File drop not delivered");
            window.update();
            require(window.files.empty(),"File drop persisted beyond its frame");
            static const char tag[]="Dialog verification";
            int userdata=42;
            ui::uiImDialog(tag,"Embedded font: 日本語 / dialog",ui::DialogLevel::Info,ui::DialogButtons::OK,&userdata);
            window.update(); window.update();
            require(ImGui::FindWindowByName(tag)!=nullptr,"Modal was not rendered");
            require(ui::uiImDialogButtonUserData(tag)==&userdata,"Dialog userdata lost");
            require(ui::uiImDialogButtonSelected(tag)==ui::DialogButtons::NONE,"Dialog selected without input");
            require(ImGui::GetDrawData()->TotalVtxCount>0,"UI produced no rendered geometry");
            if(argc>1)captureFrontBuffer(argv[1],window.width(),window.height());
            window.setThrottlingRate(2);
            int interval=0;require(SDL_GL_GetSwapInterval(&interval)&&interval==1,"SafeThrottling must use one-vblank swaps");
            window.order.clear();window.update();window.update();
            require(window.order==std::vector<char>{'U','e','u','E'},"SafeThrottling must render once per two swaps");
            window.setThrottlingRate(0);
            SDL_Event close{}; close.type=SDL_EVENT_QUIT; SDL_PushEvent(&close);
            window.update();
            require(!window.isAlive() && !window.shouldProcess() && window.closes==1,"Quit was not dispatched");
            ui::inWindowListRemove(child);
        }
        SDL_Quit();
        std::cout<<"Window callback order, SDL3 file drop/quit, embedded fonts, scale and modal rendering passed\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
