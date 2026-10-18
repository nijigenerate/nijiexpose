#include <core/nodes/projectable.hpp>
#include "nijiexpose/application.hpp"
#include "nijiexpose/plugins/package.hpp"
#include "nijiexpose/scene/package.hpp"
#include "nijiexpose/framesend.hpp"
#include "nijilive/model_io.hpp"
#include "ft/udp.hpp"
#include <bit>
#include <d3d11.h>
#include <wrl/client.h>
#include "nijiui/core/settings.hpp"
#include "nijiexpose/spout.hpp"
#include "nijilive/opengl/api.hpp"
#include "capture_frame.hpp"
#include <SDL3/SDL.h>
#include <imgui_internal.h>
#include <filesystem>
#include <iostream>
using namespace nijiexpose;
void check(bool b,const char* message){if(!b)throw std::runtime_error(message);}
int main(int argc,char** argv){try{
    check(argc==3,"Expected fixture and result directory");
    auto window=initializeApplication({argv[1]});
    auto frame=[&](int count=2){for(int i=0;i<count;++i){window->update();SDL_Delay(20);}};
    auto click=[&](float x,float y){SDL_WarpMouseInWindow(SDL_GL_GetCurrentWindow(),x,y);frame();ImGui::GetIO().AddMousePosEvent(x,y);ImGui::GetIO().AddMouseButtonEvent(0,true);frame(1);ImGui::GetIO().AddMouseButtonEvent(0,false);frame(2);};
    auto capture=[&](const char* name){auto path=(std::filesystem::path(argv[2])/name).string();captureFrontBuffer(path.c_str(),window->width(),window->height());std::cout<<"CAPTURE "<<name<<"\n";};
    frame(4);check(scene::insScene()->sceneItems.size()==1,"Command-line model load failed");
    click(window->width()/2.f,window->height()/2.f);check(scene::insScene()->selectedSceneItem()!=nullptr,"Model selection through mouse failed");
    capture("01-model-selected.png");
    std::cout<<"Selected model through application mouse interaction\n";
    if(std::getenv("NIJI_PERF_REAL")) {
        scene::neSetMeasureFPS(true);
        for(int rate:{0,1,2,6}) {
            window->setThrottlingRate(rate);
            int interval=-99;bool supported=SDL_GL_GetSwapInterval(&interval);
            frame(10);
            const auto start=SDL_GetTicksNS();size_t frames=0;
            do {window->update();++frames;}while(SDL_GetTicksNS()-start<4000000000ULL);
            const double seconds=double(SDL_GetTicksNS()-start)/1e9;
            std::cout<<"PERF viewport="<<window->width()<<"x"<<window->height()<<" requested="<<rate<<" actual="<<(supported?interval:-99)<<" frames="<<frames<<" seconds="<<seconds<<" swaps_per_second="<<frames/seconds<<" scene_fps="<<scene::neGetFPS()<<std::endl;
        }
        window->close();shutdownApplication();return 0;
    }
    if(std::getenv("NIJI_DROPDOWN_REAL")) {
        auto item=scene::insScene()->selectedSceneItem();
        check(!scene::insScene()->space->currentZone(),"Dropdown regression requires no tracking zone");
        auto ratio=std::dynamic_pointer_cast<tracking::RatioTrackingBinding>(item->bindings.at(0)->delegated);
        check(ratio && ratio->sourceName=="Head", "Expected real model Head binding");
        click(263,213);frame(3);capture("parameters-dropdown-open.png");
        check(ImGui::IsPopupOpen(nullptr,ImGuiPopupFlags_AnyPopupId|ImGuiPopupFlags_AnyPopupLevel),"Source dropdown did not open");
        click(130,297);frame(3);
        check(ratio->sourceName=="ftEyeBlinkRight" && ratio->sourceType==tracking::SourceType::Blendshape,"Blendshape dropdown selection failed");
        capture("parameters-dropdown-selected.png");
        click(263,213);frame(3);click(398,278);frame(15);click(428,371);frame(3);
        check(ratio->sourceName=="Head" && ratio->sourceType==tracking::SourceType::BoneRotYaw,"Bone submenu selection failed");
        capture("parameters-dropdown-restored.png");
        std::cout<<"Real INP, no tracking zone: source dropdown, blendshape selection and Head Yaw submenu passed\n";
        window->close();shutdownApplication();return 0;
    }
    if(std::getenv("NIJI_BACKEND_REAL")) {
        auto item=scene::insScene()->selectedSceneItem();
        capture("nijimi-real-default-fit.png");
        auto& transform=item->puppet->transform;
        transform.scale={.9f,.9f};transform.translation={0,1600,0};transform.update();item->updateTransform();
        frame(5);capture("nijimi-real-face-default.png");
        auto inspect=[&](auto&& self,const std::shared_ptr<nicxlive::core::nodes::Node>& node)->void {
            if(auto p=std::dynamic_pointer_cast<nicxlive::core::nodes::Projectable>(node)) {
                std::cout<<"Composite "<<p->name<<" initialized="<<p->initialized<<" parts="<<p->subParts.size()<<" texture="<<p->texWidth<<"x"<<p->texHeight<<" valid="<<p->hasValidOffscreenContent<<"\n";
                check(p->initialized && p->hasValidOffscreenContent && !p->subParts.empty(),"Real model composite initialization missing");
                for(auto& child:p->subParts)check(child->ignorePuppet,"Projectable child missing nijimi pre-update initialization");
            }
            for(auto& c:node->children)self(self,c);
        };
        inspect(inspect,item->puppet->root);
        item->bindings.clear();
        int eyes=0;
        for(auto& param:item->puppet->parameters)if(param->name.find("Blink(X)")!=std::string::npos){param->value={1.f,.5f};++eyes;}
        check(eyes==2,"Expected two real-model eye parameters");
        frame(5);capture("nijimi-real-eyes-closed.png");
        for(auto& param:item->puppet->parameters)if(param->name.find("Blink(X)")!=std::string::npos)param->value={0.f,.5f};
        frame(5);capture("nijimi-real-eyes-open.png");
        window->close();shutdownApplication();return 0;
    }
    if(std::getenv("NIJI_REAL_MODEL")) {
        auto item=scene::insScene()->selectedSceneItem();
        std::cout<<"Real model parameters: "<<item->puppet->parameters.size()<<" bindings: "<<item->bindings.size()<<"\n";
        frame(20);
        SDL_WarpMouseInWindow(SDL_GL_GetCurrentWindow(),700,400);frame(2);
        ImGui::GetIO().AddKeyEvent(ImGuiKey_LeftCtrl,true);frame(1);ImGui::GetIO().AddMouseWheelEvent(0,-28);frame(2);ImGui::GetIO().AddKeyEvent(ImGuiKey_LeftCtrl,false);frame(40);
        check(item->targetScale<.4f,"Real model Ctrl-wheel scaling failed");
        capture("real-model-loaded.png");
        click(44,213);SDL_WarpMouseInWindow(SDL_GL_GetCurrentWindow(),70,222);frame(20);click(150,240);frame(3);
        auto binding=item->bindings[0];auto expression=std::dynamic_pointer_cast<tracking::ExpressionTrackingBinding>(binding->delegated);
        check(expression!=nullptr,"Real model expression menu failed");
        auto enter=[&](const char* value){click(110,294);ImGui::GetIO().AddKeyEvent(ImGuiMod_Ctrl,true);ImGui::GetIO().AddKeyEvent(ImGuiKey_A,true);frame(1);ImGui::GetIO().AddKeyEvent(ImGuiKey_A,false);ImGui::GetIO().AddKeyEvent(ImGuiMod_Ctrl,false);frame(1);ImGui::GetIO().AddInputCharactersUTF8(value);frame(3);click(300,350);frame(40);};
        auto pixels=[&]{std::vector<unsigned char> p(window->width()*window->height()*3);ogl::glReadBuffer(GL_FRONT);::glReadPixels(0,0,window->width(),window->height(),GL_RGB,GL_UNSIGNED_BYTE,p.data());return p;};
        enter("-0.8");capture("real-expression-debug.png");std::cout<<"Expression: "<<expression->expr->expression()<<" parameter: "<<binding->param->value.x<<" error: "<<expression->expr->lastError()<<"\n";check(std::abs(binding->param->value.x+.8f)<.01f,"Real model negative face yaw failed");capture("real-face-left.png");auto left=pixels();
        enter("0.8");check(std::abs(binding->param->value.x-.8f)<.01f,"Real model positive face yaw failed");capture("real-face-right.png");auto right=pixels();
        size_t changed=0;for(int y=100;y<700;++y)for(int x=350;x<1000;++x){auto i=(y*window->width()+x)*3;if(std::abs(int(left[i])-right[i])+std::abs(int(left[i+1])-right[i+1])+std::abs(int(left[i+2])-right[i+2])>30)++changed;}
        check(changed>500,"Real model face yaw produced no significant rendered change");std::cout<<"Real face yaw -0.8 to +0.8: "<<changed<<" changed scene pixels\n";
        click(70,150);frame(3);auto restored=std::make_shared<scene::SceneItem>();restored->puppet=model_io::inLoadPuppet(argv[1]);check(restored->tryLoadBindings(),"Real model saved bindings missing");
        auto saved=std::dynamic_pointer_cast<tracking::ExpressionTrackingBinding>(restored->bindings[0]->delegated);check(saved&&saved->expr->expression()=="0.8","Real model save/reload lost expression");std::cout<<"Real model expression save and reload passed\n";

        window->close();shutdownApplication();return 0;
    }
    click(44,213);capture("02-binding-menu.png");
    SDL_WarpMouseInWindow(SDL_GL_GetCurrentWindow(),70,222);frame(20);capture("03-binding-types.png");
    click(150,240);frame(3);capture("04-expression-editor.png");
    auto binding=scene::insScene()->selectedSceneItem()->bindings[0];
    check(binding->type()==tracking::BindingType::ExpressionBinding,"Expression menu action did not change binding type");
    std::cout<<"Expression binding selected through menu\n";
    auto centroid=[&]{std::vector<unsigned char> px(window->width()*window->height()*4);ogl::glReadBuffer(GL_FRONT);::glReadPixels(0,0,window->width(),window->height(),GL_RGBA,GL_UNSIGNED_BYTE,px.data());double sum=0,count=0;for(int y=0;y<window->height();++y)for(int x=350;x<window->width();++x){auto i=(y*window->width()+x)*4;if(px[i]>230&&px[i+1]<40&&px[i+2]<40){sum+=x;++count;}}check(count>1000,"Rendered model pixels missing");return sum/count;};
    double before=centroid();
    click(110,294);ImGui::GetIO().AddInputCharactersUTF8("0.5");frame(3);click(300,350);frame(2);
    auto expression=std::dynamic_pointer_cast<tracking::ExpressionTrackingBinding>(binding->delegated);
    check(expression&&expression->expr->expression()=="0.5","Expression field input failed");
    check(expression->expr->lastError().empty(),"Expression compilation failed");
    check(std::abs(binding->param->value.x-.5f)<.0001f,"Expression did not update model parameter");
    auto after=centroid();std::cout<<"Rendered centroid "<<before<<" -> "<<after<<"\n";
    check(std::abs((after-before)-100)<1,"Expression did not move rendered model by expected 100 pixels");
    capture("05-expression-motion.png");
    click(38,150);frame(2);
    auto reloaded=model_io::inLoadPuppet(argv[1]);
    auto restored=std::make_shared<scene::SceneItem>();restored->puppet=reloaded;check(restored->tryLoadBindings(),"Save button did not write binding extension");
    auto saved=std::dynamic_pointer_cast<tracking::ExpressionTrackingBinding>(restored->bindings[0]->delegated);check(saved&&saved->expr->expression()=="0.5","Saved expression failed reload");
    std::cout<<"UI expression motion and UI save/reload passed\n";
    click(310,50);frame(3);
    auto control=scene::insScene()->selectedSceneItem()->animations[0];
    click(290,232);check(!control->loop,"Loop checkbox did not update animation configuration");
    click(318,200);frame(8);check(control->anim->playing()&&!control->anim->paused()&&control->anim->frame()>0,"Play button did not advance animation");
    capture("06-animation-playing.png");
    click(318,200);int pausedFrame=control->anim->frame();frame(5);check(control->anim->paused()&&control->anim->frame()==pausedFrame,"Pause button did not freeze animation frame");
    capture("07-animation-paused.png");
    click(318,200);frame(3);check(!control->anim->paused()&&control->anim->frame()>pausedFrame,"Resume button did not resume animation");
    ImGui::GetIO().AddKeyEvent(ImGuiKey_LeftShift,true);frame(1);click(286,200);ImGui::GetIO().AddKeyEvent(ImGuiKey_LeftShift,false);frame(1);
    check(!control->anim->isRunning(),"Shift-stop button did not stop animation immediately");
    click(322,135);restored->puppet=model_io::inLoadPuppet(argv[1]);restored->player=std::make_unique<animation::AnimationPlayer>(restored->puppet);
    check(restored->tryLoadAnimations()&&!restored->animations[0]->loop,"Animation save/reload lost loop setting");
    std::cout<<"UI animation play, pause, resume, stop and save/reload passed\n";
    click(154,50);frame(3);
    click(110,294);
    ImGui::GetIO().AddKeyEvent(ImGuiMod_Ctrl,true);ImGui::GetIO().AddKeyEvent(ImGuiKey_A,true);frame(1);
    ImGui::GetIO().AddKeyEvent(ImGuiKey_A,false);ImGui::GetIO().AddKeyEvent(ImGuiMod_Ctrl,false);frame(1);
    ImGui::GetIO().AddInputCharactersUTF8("BLEND('Motion')");frame(3);click(300,350);
    check(expression->expr->expression()=="BLEND('Motion')","Tracking expression input failed");
    ft::UdpSocket sender;
    auto sendBlend=[&](float value){std::vector<uint8_t> packet;auto string=[&](const char* text){while(*text)packet.push_back(*text++);packet.push_back(0);while(packet.size()%4)packet.push_back(0);};string("/VMC/Ext/Blend/Val");string(",sf");string("Motion");auto bits=std::bit_cast<uint32_t>(value);for(int shift:{24,16,8,0})packet.push_back(static_cast<uint8_t>(bits>>shift));for(int i=0;i<3;++i){check(sender.sendTo(packet,"127.0.0.1",static_cast<uint16_t>(std::stoi(std::getenv("NIJI_TEST_PORT"))))==packet.size(),"UDP packet send failed");frame(2);}};
    sendBlend(0);check(std::abs(binding->param->value.x)<.0001f,"Tracking zero did not reach model");double low=centroid();capture("08-tracking-zero.png");
    sendBlend(1);check(std::abs(binding->param->value.x-1)<.0001f,"Tracking one did not reach model");double high=centroid();check(std::abs(high-low-200)<1,"Received tracking did not move rendered model by 200 pixels");capture("09-tracking-one.png");
    std::cout<<"Actual UDP -> expression -> model -> framebuffer: "<<low<<" -> "<<high<<" pixels\n";
    click(70,150);restored->puppet=model_io::inLoadPuppet(argv[1]);check(restored->tryLoadBindings(),"Tracking save failed");saved=std::dynamic_pointer_cast<tracking::ExpressionTrackingBinding>(restored->bindings[0]->delegated);check(saved&&saved->expr->expression()=="BLEND('Motion')","Tracking expression reload failed");
    check(insCanSendFrames(),"Application Spout output unavailable");auto observer=spout::spGetSpout();char senderName[]="nijiexpose";uint32_t sw=0,sh=0;HANDLE handle=nullptr;DWORD format=0;
    check(spout::spGetSenderInfo(observer,senderName,sw,sh,handle,format)&&sw==1024&&sh==768&&handle&&format==28,"Application Spout output dimensions/format wrong");
    // Open the application's published shared texture using an independent D3D11 device.
    Microsoft::WRL::ComPtr<ID3D11Device> device;Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    check(SUCCEEDED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context)),"Spout receiver D3D device failed");
    Microsoft::WRL::ComPtr<ID3D11Texture2D> sharedTexture;
    check(SUCCEEDED(device->OpenSharedResource(handle,IID_PPV_ARGS(&sharedTexture))),"Cannot open application's Spout shared texture");
    D3D11_TEXTURE2D_DESC desc{};sharedTexture->GetDesc(&desc);desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;desc.MiscFlags=0;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> staging;check(SUCCEEDED(device->CreateTexture2D(&desc,nullptr,&staging)),"Cannot create Spout readback texture");
    context->CopyResource(staging.Get(),sharedTexture.Get());D3D11_MAPPED_SUBRESOURCE mapped{};
    check(SUCCEEDED(context->Map(staging.Get(),0,D3D11_MAP_READ,0,&mapped)),"Cannot map Spout scene pixels");
    double receivedSum=0,receivedCount=0;for(unsigned y=0;y<sh;++y){auto row=static_cast<const uint8_t*>(mapped.pData)+y*mapped.RowPitch;for(unsigned x=0;x<sw;++x)if(row[x*4]>230&&row[x*4+1]<40&&row[x*4+2]<40){receivedSum+=x;++receivedCount;}}
    context->Unmap(staging.Get(),0);check(receivedCount>1000&&std::abs(receivedSum/receivedCount-high)<1,"Spout shared texture did not contain tracking-updated model pixels");
    std::cout<<"Independent D3D11 Spout readback centroid: "<<receivedSum/receivedCount<<"\n";
    click(205,50);frame(3);capture("10-tracking-settings.png");
    click(400,332);ImGui::GetIO().AddInputCharactersUTF8("Added by UI");frame(2);click(738,332);
    check(scene::insScene()->space->getZones().size()==2&&scene::insScene()->space->getZones()[1]->name=="Added by UI","Virtual space add-zone UI failed");
    click(696,663);check(nijiui::inSettingsGetJson("space").as_object().at("zones").as_array().size()==2,"Virtual space Apply did not save new zone");
    capture("11-zone-added-and-saved.png");
    std::cout<<"Virtual space zone creation and Apply through UI passed\n";
    std::cout<<"Application Spout published actual scene at 1024x768 RGBA8\n";
    check(plugins::insPlugins.size()==3,"Application plugin enumeration lost plugins");
    click(413,50);frame(3);
    click(319,168);click(319,255);click(319,342);
    for(auto& plugin:plugins::insPlugins)plugin->callEvent("verifyUI");
    capture("12-plugin-buttons.png");std::cout<<"All three plugin UI buttons and update events verified\n";
    click(257,50);frame(3);click(360,378);click(696,663);click(743,102);frame(3);
    auto color=nijiui::inSettingsGetJson("bgColor").as_array();check(color[1].as_double()==1&&color[0].as_double()==0,"Background swatch did not update settings");
    unsigned char corner[4]{};ogl::glReadBuffer(GL_FRONT);::glReadPixels(990,20,1,1,GL_RGBA,GL_UNSIGNED_BYTE,corner);check(corner[0]==0&&corner[1]==255&&corner[2]==0,"Background swatch did not update rendered background");
    capture("13-background-green.png");
    SDL_WarpMouseInWindow(SDL_GL_GetCurrentWindow(),900,700);frame(2);
    SDL_Event drop{};drop.type=SDL_EVENT_DROP_FILE;drop.drop.windowID=SDL_GetWindowID(SDL_GL_GetCurrentWindow());drop.drop.data=argv[1];check(SDL_PushEvent(&drop),"Could not queue file-drop event");frame(3);
    check(scene::insScene()->sceneItems.size()==2,"Application file-drop did not load second model");
    click(512,384);check(scene::insScene()->selectedSceneItem()==scene::insScene()->sceneItems.back(),"Second model not selected after automatic fit");
    auto moved=scene::insScene()->selectedSceneItem();auto initial=moved->puppet->transform.translation;frame(20);
    SDL_WarpMouseInWindow(SDL_GL_GetCurrentWindow(),512,384);frame(2);ImGui::GetIO().AddMouseButtonEvent(0,true);frame(1);
    SDL_WarpMouseInWindow(SDL_GL_GetCurrentWindow(),350,480);frame(3);ImGui::GetIO().AddMouseButtonEvent(0,false);frame(20);
    check(std::abs(moved->puppet->transform.translation.x-initial.x)>100,"Model drag did not change world transform");
    capture("14-dropped-model-dragged.png");
    click(517,50);check(scene::insScene()->sceneItems.size()==1&&scene::insScene()->sceneItems.front()!=moved,"Trash button did not remove selected model");
    capture("15-selected-model-deleted.png");
    std::cout<<"Background swatch, Apply, file-drop, model drag and selected delete passed\n";
    window->close();shutdownApplication();return 0;
}catch(const std::exception& ex){std::cerr<<ex.what()<<'\n';return 1;}}




