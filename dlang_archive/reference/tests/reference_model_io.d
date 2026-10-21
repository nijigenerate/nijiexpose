module nijilive.reference_model_io;
import std.stdio;
import std.exception;
import std.file : write;
import nijilive;
import nijilive.core.animation;
import bindbc.opengl;
extern(C) {
    bool SDL_Init(uint);
    bool SDL_GL_SetAttribute(int,int);
    void* SDL_CreateWindow(const char*,int,int,ulong);
    void* SDL_GL_CreateContext(void*);
}
double clockTime() { return 1; }
void main(string[] args) {
    enforce(SDL_Init(0x20));
    SDL_GL_SetAttribute(17,3); SDL_GL_SetAttribute(18,3); SDL_GL_SetAttribute(20,1);
    auto window=SDL_CreateWindow("D model IO verification",32,32,0xA);
    enforce(window !is null); enforce(SDL_GL_CreateContext(window) !is null);
    enforce(loadOpenGL()>=GLSupport.gl33); inInit(&clockTime);
    auto pixels=new ubyte[8*8*4]; foreach(i;0..64) { pixels[i*4]=255; pixels[i*4+3]=255; }
    auto texture=new Texture(pixels,8,8,4,4,false,false);
    auto puppet=new Puppet(inCreateSimplePart(texture,null,"IO test"));
    puppet.textureSlots=[texture];
    auto param=new Parameter("Motion",false); param.uuid=123456; puppet.parameters~=[param];
    AnimationLane lane; lane.paramRef=new AnimationParameterRef(param,0);
    lane.interpolation=InterpolateMode.Linear; lane.frames=[Keyframe(0,0,.5),Keyframe(5,1,.5)];
    Animation anim; anim.length=6; anim.lanes=[lane]; anim.animationWeight=1;
    puppet.getAnimations()["Move"]=anim;
    puppet.extData["test.extension"]=cast(ubyte[])"original payload";
    write(args[1],inWriteINPPuppetMemory(puppet));
}
