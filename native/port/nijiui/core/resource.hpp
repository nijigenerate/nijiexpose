#pragma once
#include <span>
#include <cstdint>
#include <stdexcept>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
namespace nijiui {
inline std::span<const uint8_t> embeddedResource(int id) {
    auto module=GetModuleHandleW(nullptr);
    auto resource=FindResourceW(module,MAKEINTRESOURCEW(id),MAKEINTRESOURCEW(10));
    if(!resource) throw std::runtime_error("Missing embedded nijiui resource");
    auto data=LoadResource(module,resource);
    if(!data) throw std::runtime_error("Cannot load embedded nijiui resource");
    return {static_cast<const uint8_t*>(LockResource(data)),SizeofResource(module,resource)};
}
}
#else
#include <SDL3/SDL.h>
#include <filesystem>
#include <fstream>
#include <map>
#include <vector>
namespace nijiui {
inline std::span<const uint8_t> embeddedResource(int id) {
    static std::map<int,std::vector<uint8_t>> resources;
    auto found=resources.find(id);
    if(found==resources.end()) {
        const char* relative=nullptr;
        switch(id) {
        case 4201: relative="../Resources/ui/ui-info.png"; break;
        case 4202: relative="../Resources/ui/ui-warning.png"; break;
        case 4203: relative="../Resources/ui/ui-error.png"; break;
        case 4301: relative="../Resources/ui/logo.png"; break;
        case 4302: relative="../Resources/ui/icon_x256.png"; break;
        default: throw std::runtime_error("Missing nijiui resource id");
        }
        const auto* base=SDL_GetBasePath(); if(!base)throw std::runtime_error("Unable to locate application resources");
        const auto filename=std::filesystem::path(base)/relative;
        std::ifstream input(filename,std::ios::binary|std::ios::ate);
        if(!input)throw std::runtime_error("Missing bundled nijiui resource");
        const auto size=input.tellg(); std::vector<uint8_t> data(static_cast<std::size_t>(size));
        input.seekg(0); input.read(reinterpret_cast<char*>(data.data()),static_cast<std::streamsize>(size));
        if(!input)throw std::runtime_error("Cannot read bundled nijiui resource");
        found=resources.emplace(id,std::move(data)).first;
    }
    return found->second;
}
}
#endif
