module nijilive.reference_spritebatch;
import std.stdio;
import std.exception;
import nijilive;
import bindbc.opengl;
import nijiexpose.render.spritebatch;
extern(C) {
    bool SDL_Init(uint);
    bool SDL_GL_SetAttribute(int,int);
    void* SDL_CreateWindow(const char*,int,int,ulong);
    void* SDL_GL_CreateContext(void*);
}
double clockTime() { return 1; }
void main() {
    enforce(SDL_Init(0x20));
    SDL_GL_SetAttribute(17,3); SDL_GL_SetAttribute(18,3); SDL_GL_SetAttribute(20,1);
    auto window=SDL_CreateWindow("D SpriteBatch verification",64,64,0xA);
    enforce(window !is null); enforce(SDL_GL_CreateContext(window) !is null);
    enforce(loadOpenGL()>=GLSupport.gl33);
    inInit(&clockTime); inSetViewport(64,64); inSetClearColor(0,0,0,0);
    ubyte[] pixels=[255,0,0,255, 0,255,0,255, 0,0,255,255, 255,255,255,255];
    auto texture=new Texture(pixels,2,2,4,4,false,false);
    auto batch=new SpriteBatch;
    foreach(flip;0..4) foreach(rotation;[0f,.2f,-.8f]) {
        inBeginScene();
        batch.draw(texture,rect(12,18,24,16),rect.init,vec2(2,4),rotation,cast(SpriteFlip)flip,vec4(.8,.6,.7,.9));
        batch.flush();
        auto output=new ubyte[64*64*4]; inDumpViewport(output);
        ulong hash=14695981039346656037UL;
        foreach(b;output) { hash^=b; hash*=1099511628211UL; }
        writefln("pixels:%s:%016x",flip,hash);
        float[48] vertices;
        glGetBufferSubData(GL_ARRAY_BUFFER,0,vertices.sizeof,vertices.ptr);
        foreach(v;vertices) writef("%08x",*cast(uint*)&v);
        writeln();
    }
}
