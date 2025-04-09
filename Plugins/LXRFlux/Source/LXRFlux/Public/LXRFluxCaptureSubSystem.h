// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LXRFluxAnalyze.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LXRFluxCaptureSubSystem.generated.h"

class ULXRFluxLightDetector;
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

	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="LXRFlux|Capture|PostProcess")
	UMaterialInterface* IndirectPostProcessMaterial;

	void DoCaptures();

	TUniquePtr<FLXRFluxCaptureInterface> CaptureInterface;
	
	FDelegateHandle OnPostResolvedSceneColorHandle;

	bool bStopRender = false;
	TQueue<TSharedPtr<FLXRFluxAnalyzeDispatchParams>, EQueueMode::Spsc> PendingAnalyzeQueue;

	void RequestIndirectAnalyze(TSharedPtr<FLXRFluxAnalyzeDispatchParams> Params);


};

