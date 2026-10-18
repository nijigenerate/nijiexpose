#include "tracker.hpp"
#include "nijiexpose/scene/package.hpp"
#include "ext/exvmc.hpp"
#include "nijiexpose/log.hpp"
#include "nijiui/core/settings.hpp"
#include "i18n/tr.hpp"
#include <filesystem>
namespace nijiexpose::tracking {
namespace {std::filesystem::path path(const std::string& s){return std::filesystem::u8path(s);}std::string utf8(const std::filesystem::path& p){auto s=p.u8string();return {s.begin(),s.end()};}std::string join(const std::string& a,const std::string& b){return utf8(path(a)/path(b));}}
Tracker::Tracker(){std::wstring exe(32768,L'\0');auto n=GetModuleFileNameW(nullptr,exe.data(),static_cast<DWORD>(exe.size()));exe.resize(n);trackerPath=utf8(std::filesystem::path(exe).parent_path()/"nijitrack");executablePath=join(trackerPath,"nijitrack.exe");}
std::string Tracker::scriptPath()const{return join(utils::launchPath(trackerPath),trackerScriptName);}
std::string Tracker::launchConfigurationKey()const{return nativeExecutable?"native\n"+executableFilePath():"python\n"+scriptPath()+"\n"+utils::launchPath(pythonExecutablePath);}
std::string Tracker::validationError()const{
    if(nativeExecutable){if(!utils::executableFileAvailable(executableFilePath()))return i18n::_("Tracker executable is missing or cannot be executed.");}
    else{if(!utils::launchPath(pythonExecutablePath).empty()&&!utils::executableFileAvailable(pythonExecutablePath))return i18n::_("Python executable is missing or cannot be executed.");try{if(!std::filesystem::exists(path(scriptPath()))||!std::filesystem::is_regular_file(path(scriptPath())))return i18n::_("Tracker script was not found.");}catch(const std::exception&){return i18n::_("Tracker script was not found.");}}return {};
}
void Tracker::invalidateDeviceQuery(bool reset){if(queryProcess&&queryProcess->running())queryProcess->terminate();queryProcess.reset();deviceList.clear();if(reset)device=-1;deviceQueryKey=launchConfigurationKey();queryFailed=false;launchError.clear();}
std::optional<std::string> Tracker::pythonPath()const{auto configured=utils::launchPath(pythonExecutablePath);if(!configured.empty())return configured;auto bundled=join(join(join(utils::launchPath(trackerPath),".venv"),"Scripts"),"python.exe");if(std::filesystem::exists(path(bundled)))return bundled;return utils::PythonProcess<false>::detectPython();}
std::string Tracker::pythonNotFound(){return i18n::_("Python executable was not found.");}
json::Value Tracker::serialize()const{return json::Object{{"enabled",enabled},{"flipped",flipped},{"showWindow",showWindow},{"device",device},{"hostname",hostname},{"port",port},{"trackerPath",trackerPath},{"nativeExecutable",nativeExecutable},{"executablePath",executablePath},{"pythonExecutablePath",pythonExecutablePath}};}
void Tracker::deserializeFromJson(const json::Value& data){auto read=[&](const char* key,auto& value){if(auto v=json::find(data,key))json::deserialize(*v,value);};read("enabled",enabled);read("flipped",flipped);read("showWindow",showWindow);read("device",device);read("hostname",hostname);read("port",port);read("trackerPath",trackerPath);read("nativeExecutable",nativeExecutable);read("executablePath",executablePath);read("pythonExecutablePath",pythonExecutablePath);hostname=std::string(dcompat::fromStringz(hostname));trackerPath=std::string(dcompat::fromStringz(trackerPath));initialized=true;}
void Tracker::update(){if(process)process->update();if(queryProcess)queryProcess->update();}
std::vector<Tracker::DeviceInfo> Tracker::parseDeviceList(const json::Value& value){std::vector<DeviceInfo> devices;if(!value.is_array())return devices;for(auto& item:value.as_array()){auto& obj=item.as_object();auto& id=obj.at("id");devices.push_back({static_cast<int>(id.is_int64()?id.as_int64():id.as_uint64()),std::string(obj.at("name").as_string())});}return devices;}
std::vector<Tracker::DeviceInfo> Tracker::listDevices(bool force){
    auto key=launchConfigurationKey();if(key!=deviceQueryKey)invalidateDeviceQuery(!deviceQueryKey.empty());if(force){queryFailed=false;launchError.clear();}if(queryFailed)return deviceList;auto problem=validationError();if(!problem.empty()){launchError=problem;return {};}if(!deviceList.empty()&&!force)return deviceList;
    try{if(!queryProcess){queryProcess=createProcess<true>({"--list-devices"});if(!queryProcess->start())throw std::runtime_error(i18n::_("Could not start the tracker."));}else if(!queryProcess->running()){if(queryProcess->getExitCode()==0){std::string text;for(auto& line:queryProcess->stdout()){if(!text.empty())text+='\n';text+=line;}deviceList=parseDeviceList(json::parse(text));launchError.clear();if(std::none_of(deviceList.begin(),deviceList.end(),[&](auto& d){return d.id==device;}))device=-1;}else throw std::runtime_error(i18n::_("Could not list tracker cameras."));queryProcess.reset();}}
    catch(const std::exception& ex){launchError=ex.what();queryFailed=true;queryProcess.reset();insLogErr("Camera query failed: %s",ex.what());}return deviceList;
}
void Tracker::restart(){if(process&&process->running())process->terminate();process.reset();if(device<0){launchError.clear();return;}launchError=validationError();if(!launchError.empty()){insLogErr("Tracker launch rejected: %s",launchError);return;}std::vector<std::string> args{"--device",std::to_string(device),"--osc-host",hostname,"--osc-port",std::to_string(port)};if(showWindow)args.insert(args.end(),{"--show","--show-video","--show-wire"});if(flipped)args.push_back("--flip");try{process=createProcess<false>(args);if(!process->start())throw std::runtime_error(i18n::_("Could not start the tracker."));}catch(const std::exception& ex){launchError=ex.what();process.reset();insLogErr("Tracker launch failed: %s",ex.what());}}
void Tracker::terminate(){if(process&&process->running())process->terminate();if(queryProcess&&queryProcess->running())queryProcess->terminate();if(installProcess&&installProcess->running())installProcess->terminate();process.reset();queryProcess.reset();}
void Tracker::setupVSpace(){auto space=scene::insScene()->space;for(auto& zone:space->getZones())for(auto& source:zone->sources)if(source->getAdaptorName()=="VMC Receiver"&&source->getOptions().at("port")=="39540")return;auto zone=std::make_shared<VirtualSpaceZone>("Default");space->addZone(zone);zone->sources.resize(zone->sources.size()+1);auto source=std::make_shared<ExVMCAdaptor>();source->setOptions({{"port","39540"}});source->start();zone->sources.back()=source;}
std::shared_ptr<Tracker>& neTracker(){static thread_local std::shared_ptr<Tracker> tracker;if(!tracker){tracker=std::make_shared<Tracker>();if(nijiui::inSettingsCanGet("tracker"))tracker->deserializeFromJson(nijiui::inSettingsGetJson("tracker"));}return tracker;}
void neInitTracker(){if(neTracker()->enabled)neTracker()->restart();}
void neShutdownTracker(){if(neTracker()->running())neTracker()->terminate();}
}
