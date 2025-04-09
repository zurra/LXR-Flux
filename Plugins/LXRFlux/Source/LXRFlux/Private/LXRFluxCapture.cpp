#include "LXRFlux.h"

#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "FLXRFluxModule"

DEFINE_LOG_CATEGORY(LogLXRFlux);


void FLXRFluxModule::StartupModule()
{
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("LXRFlux"))->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/LXRFlux_Shaders"), PluginShaderDir);
}

void FLXRFluxModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FLXRFluxModule, FLXRFluxCapture)
