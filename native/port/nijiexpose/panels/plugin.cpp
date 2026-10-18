#include "plugin.hpp"
#include "nijiexpose/plugins/package.hpp"
#include "nijiexpose/log.hpp"
#include "nijiui/widgets/basic.hpp"
#include "i18n/tr.hpp"
namespace nijiexpose::panels {
PluginPanel::PluginPanel():Panel("Plugins",i18n::_("Plugins"),true){}
void PluginPanel::onUpdate(){using namespace nijiui;using namespace plugins;
    for(auto& plugin:insPlugins){uiImPush(&plugin);if(plugin->isEnabled()&&!plugin->hasError())if(uiImHeader(plugin->getCName(),true)){uiImPushTextWrapPos();uiImIndent();
        if(plugin->hasError()){auto text=i18n::_("%s has crashed, options are disabled.");auto pos=text.find("%s");if(pos!=std::string::npos)text.replace(pos,2,plugin->getInfo().pluginName);uiImLabelColored(text,{1,.3f,.3f,1});}
        else if(plugin->hasEvent("onRenderUI")){insPluginBegnijiui();try{plugin->callEvent("onRenderUI");}catch(const std::exception& ex){insLogErr(i18n::_("%s (plugin): %s"),plugin->getInfo().pluginId,ex.what());}insPluginEndUI();}
        else uiImLabel(i18n::_("Plugin cannot be configured."));uiImUnindent();uiImPopTextWrapPos();}uiImPop();}
}
}
