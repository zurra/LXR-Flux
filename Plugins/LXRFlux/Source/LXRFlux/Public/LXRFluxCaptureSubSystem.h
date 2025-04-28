/*
MIT License

Copyright (c) 2025 Lord Zurra

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include "CoreMinimal.h"
#include "LXRFluxAnalyze.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Materials/MaterialInterface.h"
#include "LXRFluxCaptureSubSystem.generated.h"

class ULXRFluxLightDetectorComponent;
/**
 * 
 */
UCLASS()
class LXRFLUX_API ULXRFluxSubSystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	ULXRFluxSubSystem(const FObjectInitializer& ObjectInitializer);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LXRFlux|Capture|PostProcess")
	UMaterialInterface* IndirectPostProcessMaterial;

	void DoCaptures();

	TUniquePtr<FLXRFluxCaptureInterface> CaptureInterface;

	FDelegateHandle OnPostResolvedSceneColorHandle;

	bool bStopRender = false;
	TQueue<TSharedPtr<FLXRFluxAnalyzeDispatchParams>, EQueueMode::Spsc> PendingAnalyzeQueue;

	void RequestIndirectAnalyze(TSharedPtr<FLXRFluxAnalyzeDispatchParams> Params);

	static FString GetLXRAssetPath();

	void StartABTestIndividualGISettings(UWorld* World);
};
