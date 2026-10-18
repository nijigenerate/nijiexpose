import std.stdio;
import std.exception;
import bindbc.opengl;
import nijilive.core.texture_types;
import nijilive.core.render.backends.opengl.texture_backend;
extern(C) {
    bool SDL_Init(uint flags);
    void SDL_Quit();
    bool SDL_GL_SetAttribute(int attr,int value);
    void* SDL_CreateWindow(const char*,int,int,ulong);
    void SDL_DestroyWindow(void*);
    void* SDL_GL_CreateContext(void*);
    bool SDL_GL_DestroyContext(void*);
}
void main() {
    enforce(SDL_Init(0x20));
    scope(exit) SDL_Quit();
    SDL_GL_SetAttribute(17,3);
    SDL_GL_SetAttribute(18,3);
    SDL_GL_SetAttribute(20,1);
    auto window=SDL_CreateWindow("D GL parity",32,32,0xA);
    enforce(window !is null);
    scope(exit) SDL_DestroyWindow(window);
    auto context=SDL_GL_CreateContext(window);
    enforce(context !is null);
    scope(exit) SDL_GL_DestroyContext(context);
    enforce(loadOpenGL() >= GLSupport.gl33);
    foreach (input;1..5) foreach(output;1..5) {
        GLId id;
        oglCreateTextureHandle(id);
        auto data=new ubyte[6*input];
        foreach(i,ref b;data) b=cast(ubyte)(i*17+3);
        oglUploadTextureData(id,3,2,input,output,false,data);
        auto result=new ubyte[6*output];
        oglReadTextureData(id,output,false,result);
        writef("%s:%s:",input,output);
        foreach(b;result) writef("%02x",b);
        writeln();
        foreach(filter;[Filtering.Linear,Filtering.Point]) foreach(mipmap;[false,true]) {
            oglApplyTextureFiltering(id,filter,mipmap);
            int value;
            glGetTexParameteriv(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,&value);
            writeln(value);
        }
        foreach(wrap;[Wrapping.Clamp,Wrapping.Repeat,Wrapping.Mirror]) {
            oglApplyTextureWrapping(id,wrap);
            int value;
            glGetTexParameteriv(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,&value);
            writeln(value);
        }
        oglDeleteTextureHandle(id);
        writeln(id,":",glGetError());
    }
}
