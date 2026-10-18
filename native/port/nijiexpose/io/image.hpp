#pragma once
#include <core/puppet.hpp>
#include <string>
namespace nijiexpose {
std::shared_ptr<nicxlive::core::Puppet> neLoadModelFromImage(const std::string& filename);
}
