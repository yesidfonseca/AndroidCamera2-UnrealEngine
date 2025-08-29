#include "AndroidCamera2UECoreModule.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"   // IPluginManager
#include "ShaderCore.h"                  // AddShaderSourceDirectoryMapping


IMPLEMENT_MODULE(FAndroidCamera2UECoreModule, AndroidCamera2UECore)

void FAndroidCamera2UECoreModule::StartupModule()
{
	const FString ShaderDir = FPaths::Combine(
		IPluginManager::Get().FindPlugin(TEXT("AndroidCamera2"))->GetBaseDir(),
		TEXT("Shaders")
	);

	// Alias virtual -> carpeta real
	AddShaderSourceDirectoryMapping(TEXT("/Plugin/AndroidCamera2"), ShaderDir);
}

void FAndroidCamera2UECoreModule::ShutdownModule()
{
}
