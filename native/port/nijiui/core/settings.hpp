#pragma once
#include "json.hpp"
#include <string>
#include <string_view>
namespace nijiui {
std::string inSettingsPath();
void inSettingsLoad();
void inSettingsSave();
bool inSettingsCanGet(std::string_view name);
const nijiexpose::json::Value& inSettingsGetJson(std::string_view name);
void inSettingsSetJson(std::string_view name, nijiexpose::json::Value value);
template<class T> T inSettingsGet(std::string_view name, T default_ = nijiexpose::json::initialValue<T>()) {
    if (!inSettingsCanGet(name)) return default_;
    T ot = default_;
    nijiexpose::json::deserialize(inSettingsGetJson(name), ot);
    return ot;
}
template<class T> void inSettingsSet(std::string_view name, const T& value) {
    inSettingsSetJson(name, nijiexpose::json::valueFrom(value));
}
}
