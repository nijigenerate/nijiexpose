module nijilive.reference_model_render;
import std.stdio;
import std.exception;
import nijilive;
import bindbc.opengl;
import nijilive.core.render.commands;
import nijilive.core.render.backends.opengl.runtime;
import nijilive.core.render.backends.opengl.part;
import nijilive.core.render.backends.opengl.mask;
import nijilive.core.render.backends.opengl.diff_collect_impl;
extern(C) {
    bool SDL_Init(uint);
    void SDL_Quit();
    bool SDL_GL_SetAttribute(int,int);
    void* SDL_CreateWindow(const char*,int,int,ulong);
    void SDL_DestroyWindow(void*);
    void* SDL_GL_CreateContext(void*);
    bool SDL_GL_DestroyContext(void*);
}
double clockTime() { return 1.0; }
void main() {
    enforce(SDL_Init(0x20));
    SDL_GL_SetAttribute(17,3); SDL_GL_SetAttribute(18,3); SDL_GL_SetAttribute(20,1);
    auto window=SDL_CreateWindow("D puppet render",64,64,0xA);
    enforce(window !is null);
    auto context=SDL_GL_CreateContext(window);
    enforce(context !is null);
    enforce(loadOpenGL()>=GLSupport.gl33);
    inInit(&clockTime);
    writeln("init-error:",glGetError());
    inSetViewport(64,64);
    inSetClearColor(0,0,0,0);
    auto pixels=new ubyte[16*16*4];
    foreach(i;0..256) { pixels[i*4]=255; pixels[i*4+3]=255; }
    auto texture=new Texture(pixels,16,16,4,4,false,false);
    auto part=inCreateSimplePart(texture,null,"Test quad");
    auto puppet=new Puppet(part);
    inUpdate();
    puppet.update();
    inBeginScene();
    puppet.draw();
    auto frame=new ubyte[64*64*4];
    oglDumpViewport(frame,64,64);
    size_t count;
    foreach(i;0..4096) if(frame[i*4]==255 && frame[i*4+3]==255) ++count;
    writeln("quad:",count,":",glGetError());
    auto packet=makePartDrawPacket(part,false);
    auto mp=packet;
    mp.isMask=true;
    mp.modelMatrix=mat4.scaling(.5,1,1)*packet.modelMatrix;
    MaskApplyPacket apply;
    apply.kind=MaskDrawableKind.Part;
    apply.partPacket=mp;
    foreach(dodge;[false,true]) {
        oglBeginScene();
        oglBeginMask(!dodge);
        apply.isDodge=dodge;
        oglExecuteMaskApplyPacket(apply);
        oglBeginMaskContent();
        oglDrawPartPacket(packet);
        oglEndMask();
        oglDumpViewport(frame,64,64);
        count=0;
        foreach(i;0..4096) if(frame[i*4]==255 && frame[i*4+3]==255) ++count;
        writeln("mask:",cast(int)dodge,":",count,":",glGetError());
    }
    oglBeginScene(); puppet.update(); puppet.draw();
    oglSetDifferenceAggregationEnabled(true);
    enforce(oglEvaluateDifferenceAggregation(oglGetMainAlbedo(),64,64));
    DifferenceEvaluationResult result;
    enforce(oglFetchDifferenceAggregationResult(result));
    writefln("difference:%.0f:%.0f:%.0f:%.0f:%s",result.red,result.green,result.blue,result.alpha,glGetError());
    oglEndScene();
    writeln("end-error:",glGetError());
    // The original D module owns GL resources until process exit.
}
