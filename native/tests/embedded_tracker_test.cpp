#include "embedded.h"
#ifdef NIJIEXPOSE_TEST_BUNDLED_MODEL
#include "tracking/tracker.hpp"
#endif
#include "tracking/ext/embedded_frame.hpp"
#include <SDL3/SDL.h>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
struct Session {void* value;~Session(){nt_stop(value);}};
int main(int argc,char** argv){
    try{
        require(argc==4,"Expected model, face image and output directory");
        require(SDL_Init(SDL_INIT_VIDEO),SDL_GetError());
        std::filesystem::create_directories(argv[3]);
        auto output=(std::filesystem::path(argv[3])/"inference.jsonl").string();
        auto overlay=(std::filesystem::path(argv[3])/"landmarks.ppm").string();
        std::string model=argv[1];
#ifdef NIJIEXPOSE_TEST_BUNDLED_MODEL
        if(model=="--bundled") {
            nijiexpose::tracking::Tracker tracker;
            require(tracker.validationError().empty(),tracker.validationError().c_str());
            model=tracker.modelPath();
            std::cout<<"Application-resolved model: "<<model<<std::endl;
        }
#endif
        NtConfig config{model.c_str(),0,0,0};config.image=argv[2];config.output=output.c_str();config.overlay=overlay.c_str();config.repeat=8;
        config.key_callback=[](int,unsigned,void*){};
        for(int run=0;run<2;++run){
            config.preview=run;
            char error[2048]{};Session session{nt_start(&config,error,sizeof(error))};require(session.value,error);
            int count=0;long long timestamp=-1;
            auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(20);
            for(;;){
                SDL_Event event;while(SDL_PollEvent(&event))nt_event(session.value,&event);
                NtFrame frame{};int status=nt_poll(session.value,&frame,error,sizeof(error));require(status>=0,error);
                if(status==0)break;
                if(status==2){
                    require(frame.timestamp_ms>timestamp,"Non-monotonic inference timestamp");timestamp=frame.timestamp_ms;
                    require(frame.face_present==1,"Real portrait face not detected");
                    require(frame.blend_count==56,"Expected 52 face blendshapes and four gaze values");
                    for(int i=0;i<frame.blend_count;++i)require(std::isfinite(frame.blends[i].value),"Non-finite blendshape");
                    float norm=0;for(float q:frame.quaternion){require(std::isfinite(q),"Non-finite pose");norm+=q*q;}
                    require(std::abs(norm-1)<.001f,"Invalid head quaternion");++count;
                    nijiexpose::tracking::publishEmbeddedFrame(frame);
                    nijiexpose::tracking::EmbeddedAdaptor adaptor;adaptor.start();adaptor.poll();
                    require(adaptor.isReceivingData(),"Direct adaptor did not receive inference");
                    require(adaptor.getBlendshapes().size()==56,"Direct adaptor lost blendshapes");
                    require(adaptor.getBones().at("Head").rotation.x==-frame.quaternion[2],"Direct head coordinate conversion changed");
                    NtFrame missing{};nijiexpose::tracking::publishEmbeddedFrame(missing);adaptor.poll();
                    require(!adaptor.isReceivingData() && adaptor.getBlendshapes().empty(),"Face loss left stale tracking data");
                }
                require(std::chrono::steady_clock::now()<deadline,"Inference failed to finish");
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }
            require(count>0,"No direct inference packets received");
        }
        // Cancellation joins the worker while preserving the host SDL lifetime.
        config.repeat=10000;config.preview=0;config.overlay=nullptr;config.output=nullptr;
        char error[2048]{};auto start=std::chrono::steady_clock::now();
        {Session session{nt_start(&config,error,sizeof(error))};require(session.value,error);}
        require(std::chrono::steady_clock::now()-start<std::chrono::seconds(5),"Cancellation did not join promptly");
        require(SDL_WasInit(SDL_INIT_EVENTS)!=0,"Embedded tracker shut down host SDL");
        config.model="missing-face-model.task";
        {Session session{nt_start(&config,error,sizeof(error))};require(session.value,error);int status=1;
         auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
         while(status>0 && std::chrono::steady_clock::now()<deadline){NtFrame frame{};status=nt_poll(session.value,&frame,error,sizeof(error));std::this_thread::sleep_for(std::chrono::milliseconds(2));}
         require(status==-1 && *error,"Worker error did not reach caller");}
        SDL_Quit();
        std::cout<<"PASS: real face inference, direct packets, restart, cancellation, error propagation, host SDL lifetime\n";
        return 0;
    }catch(const std::exception& ex){std::cerr<<ex.what()<<'\n';return 1;}
}
