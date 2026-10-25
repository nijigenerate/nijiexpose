#pragma once
#include "ft/adaptor.hpp"
#include <optional>
namespace nijiexpose::tracking {
std::shared_ptr<ft::Adaptor> neCreateAdaptor(const std::string& name,std::optional<std::map<std::string,std::string>> options=std::nullopt);
}
