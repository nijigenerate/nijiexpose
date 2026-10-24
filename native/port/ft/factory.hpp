#pragma once
#include "adaptor.hpp"
#include <functional>
#include <optional>
namespace nijiexpose::ft {
using AdaptorFactory=std::function<std::shared_ptr<Adaptor>()>;
void ftRegisterAdaptorFactory(const std::string& name,AdaptorFactory factory);
std::shared_ptr<Adaptor> ftCreateAdaptor(const std::string& name);
std::shared_ptr<Adaptor> ftCreateAdaptor(const std::string& name,std::map<std::string,std::string> options);
}
