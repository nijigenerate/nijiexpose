#pragma once
#include <string>
#include <vector>
namespace nijiexpose::panels {
extern thread_local std::string trackingFilter;
extern thread_local std::vector<std::string> paramNames,indexableSourceNames;
extern thread_local std::vector<float> minValues,maxValues;
extern thread_local float trackingSourceNameWidth;
void neTrackingPanelReset();
void insTrackingPanelRefresh();
}
