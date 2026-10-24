module nijilive.reference_image;
import std.stdio;
import std.exception;
import std.format;
import nijilive;
import nijiexpose.io.image;
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
    auto window=SDL_CreateWindow("D image verification",32,32,0xA);
    enforce(window !is null); enforce(SDL_GL_CreateContext(window) !is null);
    enforce(loadOpenGL()>=GLSupport.gl33); inInit(&clockTime);
    // 5-pixel rows exercise the source's default unpack alignment as well.
    glPixelStorei(GL_UNPACK_ALIGNMENT,1); glPixelStorei(GL_PACK_ALIGNMENT,1);
    foreach(channels;1..5) {
        auto file=args[1]~format("/channels-%s.png",channels);
        auto shallow=ShallowTexture(file,4);
        writef("decode:%s:%s:%s:%s:%s:",channels,shallow.width,shallow.height,shallow.channels,shallow.convChannels);
        foreach(b;shallow.data) writef("%02x",b); writeln();
        auto puppet=neLoadModelFromImage(file);
        auto part=cast(Part)puppet.root;
        auto texture=part.textures[0];
        auto data=texture.getTextureData();
        writef("crop:%s:%s:%s:%s:",channels,texture.width,texture.height,texture.channels);
        foreach(b;data) writef("%02x",b); writeln();
    }
}
