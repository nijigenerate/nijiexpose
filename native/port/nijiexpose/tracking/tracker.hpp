#pragma once
#include "nijiexpose/utils/subprocess.hpp"
#include "nijiexpose/utils/launchpath.hpp"
#include "nijiexpose/json.hpp"
#include <memory>
namespace nijiexpose::tracking {
class Tracker {
    std::string deviceQueryKey;
    bool queryFailed=false;
public:
    bool enabled=false,flipped=true,showWindow=false;
    std::string hostname="127.0.0.1";
    uint32_t port=39540;
    int device=-1;
    std::string trackerPath;
    bool nativeExecutable=false;
    std::string executablePath,pythonExecutablePath,launchError;
    std::string trackerScriptName="nijitrack.py";
    static constexpr auto ScriptDownloadSource="https://github.com/nijigenerate/nijitrack/archive/refs/heads/main.zip";
    bool initialized=false;
    std::shared_ptr<utils::SubProcess<false>> process;
    std::shared_ptr<utils::SubProcess<true>> queryProcess,installProcess;
    struct DeviceInfo {int id=0;std::string name;};
    std::vector<DeviceInfo> deviceList;
    Tracker();
    std::string scriptPath() const;
    std::string executableFilePath() const {return utils::launchPath(executablePath);}
    std::string launchConfigurationKey() const;
    std::string validationError() const;
    void invalidateDeviceQuery(bool resetSelection=true);
    std::optional<std::string> pythonPath() const;
    json::Value serialize() const;
    void deserializeFromJson(const json::Value& data);
    void update();
    bool running() const{return process&&process->running();}
    std::vector<DeviceInfo> parseDeviceList(const json::Value& value);
    std::vector<DeviceInfo> listDevices(bool force=false);
    void restart();
    template<bool capture> std::shared_ptr<utils::SubProcess<capture>> createProcess(std::vector<std::string> args){
        auto error=validationError();if(!error.empty())throw std::runtime_error(error);
        if(nativeExecutable){if(executableFilePath().empty())throw std::runtime_error("Tracker executable path is empty");return std::make_shared<utils::SubProcess<capture>>(executableFilePath(),std::move(args));}
        auto interpreter=pythonPath();if(!interpreter||interpreter->empty())throw std::runtime_error(pythonNotFound());
        return std::make_shared<utils::PythonProcess<capture>>(scriptPath(),std::move(args),interpreter);
    }
    static std::string pythonNotFound();
    void terminate();
    std::shared_ptr<utils::SubProcess<true>> install();
    void setupVSpace();
};
std::shared_ptr<Tracker>& neTracker();
void neInitTracker();
void neShutdownTracker();
}
