#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <span>
#include <cstdint>
#include <stdexcept>
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
