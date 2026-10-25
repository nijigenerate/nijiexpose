#pragma once
#include <filesystem>
#include <string>

namespace niji {
inline std::filesystem::path path_from_utf8(const std::string &text) {
  return std::filesystem::path(std::u8string(text.begin(), text.end()));
}
inline std::string path_to_utf8(const std::filesystem::path &path) {
  const auto text = path.u8string();
  return std::string(text.begin(), text.end());
}
} // namespace niji
