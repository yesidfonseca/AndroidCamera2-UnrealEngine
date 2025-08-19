#pragma once
#include "Modules/ModuleManager.h"



class FAndroidCamera2UECoreModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
   
};