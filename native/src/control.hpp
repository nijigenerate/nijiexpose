#pragma once
#include <filesystem>
namespace nijiexpose::control {
// Optional local command inbox. Commands run on the application's render thread.
void start(const std::filesystem::path& directory);
void poll();
}
