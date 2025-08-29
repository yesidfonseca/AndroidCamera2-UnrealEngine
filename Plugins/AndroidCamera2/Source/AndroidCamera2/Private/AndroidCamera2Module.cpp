#include "IAndroidCamera2Module.h"


#define LOCTEXT_NAMESPACE "FAndroidCamera2Module"

class FAndroidCamera2Module
	: public IAndroidCamera2Module
{
	virtual void StartupModule() override
	{
		
	}

	virtual void ShutdownModule() override
	{
		// Aquí va la limpieza del módulo
	}
};

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAndroidCamera2Module, AndroidCamera2)