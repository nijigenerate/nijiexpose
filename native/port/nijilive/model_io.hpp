#pragma once
#include <core/puppet.hpp>
#include <span>
#include <string>
namespace nijiexpose::model_io {
using nicxlive::core::Puppet;
bool inIsINPMode();
std::shared_ptr<Puppet> inLoadPuppet(const std::string& file);
std::shared_ptr<Puppet> inLoadPuppetFromMemory(std::span<const uint8_t> jsonData);
std::shared_ptr<Puppet> inLoadJSONPuppet(const std::string& jsonData);
std::shared_ptr<Puppet> inLoadINPPuppet(std::span<const uint8_t> buffer);
void inWriteINPExtensions(const std::shared_ptr<Puppet>& puppet,const std::string& file);
}
