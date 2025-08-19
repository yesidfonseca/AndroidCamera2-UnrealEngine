#pragma once

#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

class IAndroidCamera2Module : public IModuleInterface
{
public:
	static inline IAndroidCamera2Module& Get()
	{
		return FModuleManager::LoadModuleChecked<IAndroidCamera2Module>("AndroidCamera2");
	}

	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("AndroidCamera2");
	}
};