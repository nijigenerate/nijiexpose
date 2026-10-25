#pragma once
#include "sdl_media.hpp"
#include "tracking.hpp"
#include "face_connections.hpp"
#include <cmath>
#include <iomanip>
#include <mutex>
#include <sstream>
namespace niji {
struct DisplayOptions {
  bool video=false, wire=false, text=false, blend=false;
  void toggle(unsigned key) {
    switch (key) {
    case 'v': video=!video; break;
    case 'w': wire=!wire; break;
    case 't': text=!text; break;
    case 'b': blend=!blend; break;
    }
  }
};
class SharedDisplayOptions {
  std::mutex mutex_;
  DisplayOptions options_;
public:
  explicit SharedDisplayOptions(DisplayOptions options):options_(options) {}
  DisplayOptions get() { std::lock_guard lock(mutex_); return options_; }
  void set(DisplayOptions value) { std::lock_guard lock(mutex_); options_=value; }
};
struct PreviewLabel { float x, y; std::string text; SDL_Color color; };
struct PreviewFrame {
  int width=0, height=0;
  std::vector<uint8_t> rgb;
  std::vector<SDL_Vertex> vertices;
  std::vector<int> indices;
  std::vector<PreviewLabel> labels;
  void quad(SDL_FPoint a, SDL_FPoint b, SDL_FPoint c, SDL_FPoint d, SDL_FColor color) {
    const int base=static_cast<int>(vertices.size());
    for (auto p : {a,b,c,d}) vertices.push_back({p,color,{0,0}});
    for (int i : {0,1,2,0,2,3}) indices.push_back(base+i);
  }
  void rect(float x,float y,float w,float h,SDL_FColor color) {
    quad({x,y},{x+w,y},{x+w,y+h},{x,y+h},color);
  }
  void line(SDL_FPoint a,SDL_FPoint b,SDL_FColor color) {
    a.x+=0.5f; a.y+=0.5f; b.x+=0.5f; b.y+=0.5f;
    const float length=std::hypot(b.x-a.x,b.y-a.y);
    if (length < 0.001f) return;
    const float dx=-(b.y-a.y)*0.5f/length,dy=(b.x-a.x)*0.5f/length;
    quad({a.x+dx,a.y+dy},{b.x+dx,b.y+dy},{b.x-dx,b.y-dy},{a.x-dx,a.y-dy},color);
  }
};
// Build an owned render packet on the inference thread. SDL rendering stays on main.
inline PreviewFrame render_preview(const uint8_t *rgb,int width,int height,
    const MpFaceLandmarkerResult &result,const Pose *pose,const DisplayOptions &options) {
  if (!rgb || width<1 || height<1 || width>8192 || height>8192) throw std::runtime_error("Invalid preview dimensions");
  PreviewFrame display;
  display.width=width; display.height=height;
  display.rgb.resize(static_cast<size_t>(width)*height*3);
  if (options.video) std::memcpy(display.rgb.data(),rgb,display.rgb.size());
  if (options.wire && result.face_landmarks_count) {
    const auto &landmarks=result.face_landmarks[0];
    auto point=[&](unsigned i) {
      return SDL_FPoint{landmarks.landmarks[i].x*width,landmarks.landmarks[i].y*height};
    };
    auto valid=[](SDL_FPoint p) { return std::isfinite(p.x) && std::isfinite(p.y); };
    for (const auto &edge : face_connections) {
      if (edge[0]>=landmarks.landmarks_count || edge[1]>=landmarks.landmarks_count) continue;
      auto a=point(edge[0]),b=point(edge[1]);
      if (valid(a) && valid(b)) display.line(a,b,{0,1,0,1});
    }
    for (unsigned i=0;i<landmarks.landmarks_count;++i) {
      auto p=point(i);
      if (valid(p)) display.rect(p.x-1,p.y-1,3,3,{1,0,0,1});
    }
  }
  if (options.text && pose) {
    auto label=[&](float y,const std::string &text) { display.labels.push_back({10,y,text,{255,0,0,255}}); };
    std::ostringstream text;
    text<<std::fixed<<std::setprecision(1)<<"Yaw: "<<pose->euler[0]<<"  Pitch: "<<pose->euler[1]<<"  Roll: "<<pose->euler[2];
    label(20,text.str()); text.str("");
    text<<std::setprecision(3)<<"Pos X: "<<pose->nose[0]<<" Y: "<<pose->nose[1]<<" Z: "<<pose->nose[2];
    label(40,text.str());
    for (int i=0;i<2;++i) {
      const auto &gaze=i==0?pose->gaze_right:pose->gaze_left;
      text.str(""); text<<std::setprecision(2)<<"Gaze "<<(i==0?"R":"L")<<": x="<<gaze[0]<<" y="<<gaze[1];
      label(static_cast<float>(60+i*20),text.str());
    }
  }
  if (options.blend && result.face_blendshapes_count) {
    const auto &blend=result.face_blendshapes[0];
    for (unsigned i=0;i<blend.categories_count;++i) {
      const auto &category=blend.categories[i];
      const float y=120.0f+static_cast<float>(i)*10;
      const float score=std::isfinite(category.score)?std::clamp(category.score,0.f,1.f):0.f;
      display.rect(static_cast<float>(width-160),y,score*150,6,{1,1,0,1});
      std::ostringstream text;
      text<<(category.category_name?category.category_name:"")<<": "<<std::fixed<<std::setprecision(2)<<score;
      display.labels.push_back({static_cast<float>(width-360),y,text.str(),{255,255,255,255}});
    }
  }
  return display;
}
class Preview {
  std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)> window_{nullptr,SDL_DestroyWindow};
  std::unique_ptr<SDL_Renderer,decltype(&SDL_DestroyRenderer)> renderer_{nullptr,SDL_DestroyRenderer};
  std::unique_ptr<SDL_Texture,decltype(&SDL_DestroyTexture)> texture_{nullptr,SDL_DestroyTexture};
  int width_=0,height_=0;
  bool open_=true;
public:
  DisplayOptions options;
  explicit Preview(DisplayOptions value,bool hidden=false):options(value) {
    window_.reset(SDL_CreateWindow("MediaPipe Face",640,480,SDL_WINDOW_RESIZABLE | (hidden?SDL_WINDOW_HIDDEN:0)));
    sdl_check(window_!=nullptr,"create preview window");
    renderer_.reset(SDL_CreateRenderer(window_.get(),nullptr));
    sdl_check(renderer_!=nullptr,"create preview renderer");
  }
  bool event(const SDL_Event& event) {
    const auto id=SDL_GetWindowID(window_.get());
    if(event.type==SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID==id) {open_=false;return true;}
    if(event.type==SDL_EVENT_KEY_DOWN && event.key.windowID==id) {
      if(!event.key.repeat) options.toggle(static_cast<unsigned>(event.key.key));
      return true;
    }
    return false;
  }
  bool open() const {return open_;}
  void draw(const PreviewFrame &frame) {
    if (!open_) return;
    if (!texture_ || width_!=frame.width || height_!=frame.height) {
      texture_.reset(SDL_CreateTexture(renderer_.get(),SDL_PIXELFORMAT_RGB24,SDL_TEXTUREACCESS_STREAMING,frame.width,frame.height));
      sdl_check(texture_!=nullptr,"create preview texture");
      width_=frame.width; height_=frame.height;
      sdl_check(SDL_SetRenderLogicalPresentation(renderer_.get(),width_,height_,SDL_LOGICAL_PRESENTATION_LETTERBOX),"set preview dimensions");
    }
    sdl_check(SDL_UpdateTexture(texture_.get(),nullptr,frame.rgb.data(),frame.width*3),"upload preview frame");
    sdl_check(SDL_SetRenderDrawColor(renderer_.get(),0,0,0,255),"set preview color");
    sdl_check(SDL_RenderClear(renderer_.get()),"clear preview");
    sdl_check(SDL_RenderTexture(renderer_.get(),texture_.get(),nullptr,nullptr),"draw camera texture");
    if (!frame.vertices.empty())
      sdl_check(SDL_RenderGeometry(renderer_.get(),nullptr,frame.vertices.data(),static_cast<int>(frame.vertices.size()),frame.indices.data(),static_cast<int>(frame.indices.size())),"draw tracking geometry");
    for (const auto &label:frame.labels) {
      sdl_check(SDL_SetRenderDrawColor(renderer_.get(),label.color.r,label.color.g,label.color.b,label.color.a),"set label color");
      sdl_check(SDL_RenderDebugText(renderer_.get(),label.x,label.y,label.text.c_str()),"draw tracking label");
    }
  }
  void present(const PreviewFrame &frame) { draw(frame); if (open_) sdl_check(SDL_RenderPresent(renderer_.get()),"present preview"); }
  Surface readback() { return surface(SDL_RenderReadPixels(renderer_.get(),nullptr)); }
  SDL_WindowID window_id() const { return SDL_GetWindowID(window_.get()); }
};
} // namespace niji
