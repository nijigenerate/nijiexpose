#include "package.hpp"
#include "exvmc.hpp"
#include "ft/factory.hpp"
namespace nijiexpose::tracking {
std::shared_ptr<ft::Adaptor> neCreateAdaptor(const std::string& name,std::optional<std::map<std::string,std::string>> options) {
    if(name=="VMC Receiver") return std::make_shared<ExVMCAdaptor>();
    if(!options) return ft::ftCreateAdaptor(name);
    return ft::ftCreateAdaptor(name,std::move(*options));
}
}
