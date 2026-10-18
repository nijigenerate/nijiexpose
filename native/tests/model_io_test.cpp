#include "nijilive/model_io.hpp"
#include "nijilive/animation/player.hpp"
#include "nijilive/opengl/backend.hpp"
#include <core/runtime_state.hpp>
#include <SDL3/SDL.h>
#include <fstream>
#include <iostream>
void check(bool value,const char* message) { if(!value) throw std::runtime_error(message); }
std::vector<uint8_t> read(const char* file) { std::ifstream in(file,std::ios::binary); return {(std::istreambuf_iterator<char>(in)),std::istreambuf_iterator<char>()}; }
int main(int argc,char** argv) {
    namespace core=nicxlive::core; namespace io=nijiexpose::model_io;
    try {
        check(argc==2,"Expected working copy of original D model");
        check(SDL_Init(SDL_INIT_VIDEO),SDL_GetError());
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3); SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);
        auto* window=SDL_CreateWindow("Model IO verification",32,32,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);
        check(window,SDL_GetError()); check(SDL_GL_CreateContext(window),SDL_GetError());
        auto backend=std::make_shared<nijiexpose::ogl::Backend>(); core::inSetRenderBackend(backend); backend->initializeRenderer();
        core::inSetViewport(32,32); core::inSetClearColor(0,0,0,0);
        struct DrawableInitProbe final : core::nodes::Drawable {
            bool resourcesReady=false, setupReady=false;
            void finalizeDrawable() override { resourcesReady=true; }
            void setupSelf() override { check(resourcesReady,"setupSelf ran before drawable initialization"); setupReady=true; }
        };
        auto initProbe=std::make_shared<DrawableInitProbe>();
        std::shared_ptr<core::nodes::Node> initNode=initProbe;
        initNode->finalize();
        check(initProbe->resourcesReady && initProbe->setupReady,"Model finalize omitted drawable initialization before first update");
        // Resolve references without importing D's destructive seam "repair".
        for(bool reciprocal : {false,true}) {
            auto root=std::make_shared<core::nodes::Node>();
            auto seam=std::make_shared<core::Puppet>(root);
            auto a=std::make_shared<core::nodes::Drawable>();
            auto b=std::make_shared<core::nodes::Drawable>();
            a->uuid=901; b->uuid=902;
            a->setParent(root); b->setParent(root);
            root->children={a,b};
            a->setPuppet(seam); b->setPuppet(seam);
            a->mesh->vertices={{0,0},{1,0},{2,0}};
            b->mesh->vertices={{0,1},{1,1},{2,1}};
            const std::vector<std::ptrdiff_t> authored={2,-1,0};
            a->weldedLinks.push_back({b->uuid,{},authored,.25f});
            if(reciprocal)b->weldedLinks.push_back({a->uuid,{},authored,.75f});
            // Runtime buffers are not yet built. Repeated finalization must be harmless.
            a->finalize(); b->finalize(); a->finalize();
            check(a->weldedLinks.size()==1,"Finalize deleted an authored welding link");
            check(a->weldedLinks[0].target.lock()==b,"Welding endpoint not resolved");
            check(a->weldedLinks[0].indices==authored,"Finalize rewrote authored welding indices");
            check(a->weldedLinks[0].weight==.25f,"Finalize changed authored welding weight");
            check(b->weldedLinks.size()==(reciprocal?1u:0u),"Finalize invented a reciprocal welding link");
        }
        auto original=read(argv[1]);
        auto puppet=io::inLoadPuppet(argv[1]);
        check(io::inIsINPMode(),"INP mode not retained");
        check(puppet->textureSlots.size()==1,"Texture slots missing");
        auto part=std::dynamic_pointer_cast<core::nodes::Part>(puppet->root);
        check(part && part->textures[0]==puppet->textureSlots[0],"Part texture not resolved at load time");
        check(puppet->parameters.size()==1 && puppet->findParameter(123456),"Parameter not loaded");
        check(puppet->animations.size()==1,"Animation missing");
        auto anim=std::dynamic_pointer_cast<nijiexpose::animation::Animation>(puppet->animations.at("Move"));
        check(anim && anim->lanes.size()==1,"Animation lanes missing");
        check(anim->lanes[0].paramRef->targetParam==puppet->findParameter(123456),"Animation target not finalized");
        check(anim->lanes[0].get(2.5f)==.5f,"Loaded interpolation differs");
        nijiexpose::animation::AnimationPlayer player(puppet);
        auto playback=player.play("Move"); player.update(anim->timestep); check(playback->frame()==1,"Loaded animation did not advance");
        puppet->update(); backend->beginScene(); puppet->draw();
        std::vector<uint8_t> frame(32*32*4); backend->dumpViewport(frame,32,32);
        size_t red=0; for(size_t i=0;i<frame.size();i+=4) if(frame[i]==255 && frame[i+3]==255) ++red;
        check(red==64,"Loaded D model did not render its 8x8 part");
        auto& ext=puppet->extData.at("test.extension");
        check(std::string(ext.begin(),ext.end())=="original payload","Extended data not loaded");
        io::inWriteINPExtensions(puppet,argv[1]);
        check(read(argv[1])==original,"Unchanged extension write altered original D model bytes");
        // Preserve bytes following EXT, including when deleting the section.
        { std::ofstream out(argv[1],std::ios::binary|std::ios::app); out<<"trailer"; }
        puppet->extData["test.extension"]={'n','e','w'};
        io::inWriteINPExtensions(puppet,argv[1]);
        auto reloaded=io::inLoadPuppet(argv[1]);
        check(reloaded->extData.at("test.extension")==std::vector<uint8_t>{'n','e','w'},"Edited extension not persisted");
        puppet->extData.clear(); io::inWriteINPExtensions(puppet,argv[1]);
        auto removed=read(argv[1]);
        check(std::string(removed.end()-7,removed.end())=="trailer","Trailing data lost when deleting EXT section");
        check(io::inLoadPuppet(argv[1])->extData.empty(),"Deleted extension still present");
        std::cout<<"D-written INP: texture, mesh, parameter, animation playback/rendering and extension rewrite passed\n";
        return 0;
    } catch(const std::exception& error) { std::cerr<<error.what()<<'\n'; return 1; }
}
