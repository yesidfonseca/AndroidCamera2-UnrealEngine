#include "Modules/ModuleManager.h"
class FQuircModule : public IModuleInterface
{
public:
    virtual void StartupModule() override {}
    virtual void ShutdownModule() override {}
};
IMPLEMENT_MODULE(FQuircModule, Quirc)