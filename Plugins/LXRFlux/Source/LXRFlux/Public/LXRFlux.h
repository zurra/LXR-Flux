#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

#define NUM_THREADS_X 32
#define NUM_THREADS_Y 32
#define NUM_THREADS_Z 1
#define LUMINANCE_SCALE 10000.0f
#define LUMINANCE_THRESHOLD 0.05f


DECLARE_LOG_CATEGORY_EXTERN(LogLXRFlux, Log, All);

class FLXRFluxModule: public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
