// Copyright Clusterfact Games 2023.  All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MotionDelayBuffer.h"
#include "RHIGPUReadback.h"
#include "Components/SceneCaptureComponent2D.h"
#include "UnrealClient.h"
#include "Components/ActorComponent.h"
#include "RenderCommandFence.h"
#include "LXRFluxLightDetector.generated.h"


struct FLXRFluxAnalyzeDispatchParams;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LXRFLUX_API ULXRFluxLightDetector : public UActorComponent
{
	GENERATED_BODY()

public:
	ULXRFluxLightDetector();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void RequestOneShotCaptureUpdate();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXRFlux|Detection|Indirect")
	bool bCaptureIndirect = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXRFlux|Detection|Direct")
	bool bCaptureDirect = true;


	UFUNCTION(BlueprintPure, Category="LXRFlux|Detection|Indirect")
	UTextureRenderTarget2D* GetBotTarget() const { return BotRT; }

	UFUNCTION(BlueprintPure, Category="LXRFlux|Detection|Indirect")
	UTextureRenderTarget2D* GetTopTarget() const { return TopRT; }

	/**
	 * Size of the RenderTextures
	 *
	 * Default 32.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXRFlux|Detection|Indirect")
	int RenderTextureSize = 32;


	/**
	* Override default Top Render Texture
	*
	* Default RTF_RGBA8 RenderTextureSize*RenderTextureSize
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LXRFlux|Detection|Indirect")
	TObjectPtr<UTextureRenderTarget2D> TopRT;

	/**
	* Override default Bot Render Texture
	*
	* Default RTF_RGBA8 RenderTextureSize*RenderTextureSize  
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LXRFlux|Detection|Indirect")
	TObjectPtr<UTextureRenderTarget2D> BotRT;

	UPROPERTY(BlueprintReadOnly, Category="LXRFlux|Detection|Indirect")
	float Luminance;

	UPROPERTY(BlueprintReadOnly, Category="LXRFlux|Detection|Indirect")
	FLinearColor Color;

private:
	void PrepareChildActor();
	void HandleOneShotSceneCaptureCooldown();
	void CreateRenderTargets();
	void CreateCapturePrerequisites();
	void UpdatePostprocessingLumenSettings();
	void FetchLumenCVars(IConsoleVariable* Var);

	void BindCvarDelegates();


	UPROPERTY()
	TWeakObjectPtr<UUserWidget> IndirectDebugWidget;

	UPROPERTY()
	TObjectPtr<USceneCaptureComponent2D> TopSceneCaptureComponent;

	UPROPERTY()
	TObjectPtr<USceneCaptureComponent2D> BotSceneCaptureComponent;

	UPROPERTY()
	TObjectPtr<UChildActorComponent> ChildActorComponent;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> IndirectMeshComponent;

	UPROPERTY()
	TArray<TObjectPtr<USceneCaptureComponent2D>> SceneCaptures;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> IndirectMatInst;

	bool bFLXRFluxDisabled = true;
	bool bFLXRFluxCaptureDisabled = true;
	bool bFLXRFluxDebugCaptureWidgetEnabled = true;
	bool bFLXRFluxDebugMeshEnabled = true;

	float bFLXRFluxLumenSceneLightingQuality = 1;
	float bFLXRFluxLumenSceneDetail = 1;
	float bFLXRFluxLumenSceneViewDistance = 5000;
	float bFLXRFluxLumenFinalGatherQuality = 1;
	float bFLXRFluxLumenMaxTraceDistance = 5000;
	float bFLXRFluxLumenSurfaceCacheResolution = 0.5;
	float bFLXRFluxLumenSceneLightingUpdateSpeed = 1;
	float bFLXRFluxLumenFinalGatherLightingUpdateSpeed = 1;
	float bFLXRFluxLumenDiffuseColorBoost = 1;
	float bFLXRFluxLumenSkylightLeaking = 0;
	float bFLXRFluxLumenFullSkylightLeakingDistance = 1000;

	float LumenCVarsSum;
	int NewLumenCVarSum = 0;
	TArray<float*> LumenCVars;

	FVector TargetLocation;
	TArray<TObjectPtr<UTextureRenderTarget2D>> RenderTargets;
	TSharedPtr<FLXRFluxAnalyzeDispatchParams> IndirectAnalyzeDispatchParams;
	TCircularHistoryBuffer<float> IndirectLuminanceHistory;
	TCircularHistoryBuffer<FLinearColor> IndirectColorHistory;

	float RemoveCaptureEveryFrameTimer;
	bool bNeedsSceneCaptureUpdate;

	void OnReadbackComplete(float Luminance, FLinearColor InColor);
	float GetAverageLuminance() const;
	FLinearColor GetAverageColor() const;

};
