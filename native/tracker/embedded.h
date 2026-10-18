#pragma once
#ifdef _WIN32
#ifdef NIJITRACK_EMBEDDED_EXPORTS
#define NT_API __declspec(dllexport)
#else
#define NT_API __declspec(dllimport)
#endif
#else
#define NT_API __attribute__((visibility("default")))
#endif
// Ownership stays inside the module; no STL objects cross CRT boundaries.
struct NtConfig {
    const char* model;
    int device,flip,preview;
    int width=640,height=480;
    double fps=30;
    const char* image=nullptr; // deterministic integration testing without a camera
    const char* output=nullptr;
    const char* overlay=nullptr;
    int repeat=1;
    void(*key_callback)(int,unsigned,void*)=nullptr;
    void* key_context=nullptr;
};
struct NtBlend {char name[128];float value;};
struct NtFrame {
    long long timestamp_ms;
    int face_present,blend_count;
    float position[3],quaternion[4];
    NtBlend blends[64];
};
extern "C" {
NT_API void* nt_start(const NtConfig*,char*,int) noexcept;
NT_API int nt_poll(void*,NtFrame*,char*,int) noexcept;
NT_API int nt_event(void*,const void*) noexcept;
NT_API void nt_stop(void*) noexcept;
NT_API int nt_devices(void(*)(int,const char*,void*),void*,char*,int) noexcept;
}
