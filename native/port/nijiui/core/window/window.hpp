#pragma once
#include <memory>
#include <vector>
namespace nijiui {
class InWindow {
protected:
    bool showUI=true;
    virtual void onOpened() {}
    virtual void onClosed() {}
    virtual void onResized(int,int) {}
    virtual bool shouldProcess() { return false; }
public:
    virtual ~InWindow()=default;
    virtual void onEarlyUpdate()=0;
    virtual void onUpdate()=0;
    virtual void focus()=0;
    virtual void close()=0;
    virtual bool isAlive()=0;
    virtual int width()=0;
    virtual int height()=0;
};
std::vector<std::shared_ptr<InWindow>>& inWindowListGet();
void inWindowListAdd(std::shared_ptr<InWindow> window);
void inWindowListRemove(const std::shared_ptr<InWindow>& window);
}
