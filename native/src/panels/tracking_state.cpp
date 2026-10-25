#include "tracking_state.hpp"
#include "scene/package.hpp"
namespace nijiexpose::panels {
thread_local std::string trackingFilter;
thread_local std::vector<std::string> paramNames,indexableSourceNames;
thread_local std::vector<float> minValues,maxValues;
thread_local float trackingSourceNameWidth=0;
void neTrackingPanelReset(){indexableSourceNames.clear();minValues.clear();maxValues.clear();}
void insTrackingPanelRefresh(){trackingFilter.clear();paramNames.clear();if(auto item=scene::insScene()->selectedSceneItem())for(auto& binding:item->bindings)paramNames.push_back(binding->param->name);}
}
