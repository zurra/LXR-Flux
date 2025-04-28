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
#include "MotionDelayBuffer.h"
#include "RHIGPUReadback.h"
#include "Components/SceneCaptureComponent2D.h"
#include "UnrealClient.h"
#include "Components/ActorComponent.h"
#include "RenderCommandFence.h"
#include "LXRFluxLightDetector.generated.h"

UENUM()
enum ESmoothMethod
{
	TargetValueOverTime,
	HistoryBuffer,
	ValidateDelta,
	None
};

enum EValueToGet
{
	Top,
	Bot,
	Combined
};

struct FLXRValidatedValue
{
	FLXRValidatedValue(int InRequiredStableFrames,
	                   float InThresholdDelta)
	{
		RequiredStableFrames = InRequiredStableFrames;
		ThresholdDelta = InThresholdDelta;
	}

	int ValidCounter = 0;
	int RequiredStableFrames = 2;
	float ThresholdDelta = 0.05f;

	FLXRValidatedValue() = default;

};

struct FLXRValidatedFloat : FLXRValidatedValue
{
	float LastValidValue = 0.0f;
	float PendingCandidate = 0.0f;

	FLXRValidatedFloat(int InRequiredStableFrames, float InThresholdDelta)
	: FLXRValidatedValue(InRequiredStableFrames, InThresholdDelta)
	{}
	
	float Tick(const float NewValue)
	{
		const float Delta = FMath::Abs(NewValue - LastValidValue);

		if (Delta < ThresholdDelta)
		{
			// Close enough, accept immediately
			LastValidValue = NewValue;
			ValidCounter = 0;
			PendingCandidate = 0.0f;
		}
		else
		{
			// Big change, validate over time
			if (FMath::IsNearlyEqual(NewValue, PendingCandidate, ThresholdDelta))
			{
				ValidCounter++;
				if (ValidCounter >= RequiredStableFrames)
				{
					LastValidValue = NewValue;
					ValidCounter = 0;
					PendingCandidate = 0.0f;
				}
			}
			else
			{
				// New candidate
				PendingCandidate = NewValue;
				ValidCounter = 1;
			}
		}

		return LastValidValue;
	}
};

struct FLXRValidatedColor : FLXRValidatedValue
{
	FLinearColor LastValidValue = FLinearColor::Black;
	FLinearColor PendingCandidate = FLinearColor::Black;

	FLXRValidatedColor(int InRequiredStableFrames, float InThresholdDelta)
	: FLXRValidatedValue(InRequiredStableFrames, InThresholdDelta)
	{}
	
	FLinearColor Tick(const FLinearColor& NewValue)
	{
		const float Delta = FLinearColor::Dist(LastValidValue, NewValue);

		if (Delta < ThresholdDelta)
		{
			// Close enough, accept immediately
			LastValidValue = NewValue;
			ValidCounter = 0;
			PendingCandidate = FLinearColor::Black;
		}
		else
		{
			// Big change, validate over time
			if (FLinearColor::Dist(NewValue, PendingCandidate) < ThresholdDelta)
			{
				ValidCounter++;
				if (ValidCounter >= RequiredStableFrames)
				{
					LastValidValue = NewValue;
					ValidCounter = 0;
					PendingCandidate = FLinearColor::Black;
				}
			}
			else
			{
				// New candidate
				PendingCandidate = NewValue;
				ValidCounter = 1;
			}
		}

		return LastValidValue;
	}
};


USTRUCT(Blueprintable)
struct FFluxOutput
{
	GENERATED_BODY()
	FFluxOutput(): Luminance(0), Color(), TopLuminance(0), TopColor(), BotLuminance(0), BotColor()
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LXRFlux")
	float Luminance;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LXRFlux")
	FLinearColor Color;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LXRFlux")
	float TopLuminance;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LXRFlux")
	FLinearColor TopColor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LXRFlux")
	float BotLuminance;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LXRFlux")
	FLinearColor BotColor;
};


struct FLXRFluxAnalyzeDispatchParams;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LXRFLUX_API ULXRFluxLightDetectorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULXRFluxLightDetectorComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void RequestOneShotCaptureUpdate();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXRFlux|Capture")
	bool bCaptureIndirect = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="LXRFlux|Capture")
	bool bCaptureDirect = true;

	UPROPERTY(EditAnywhere, Category="LXRFlux|Analyze")
	bool bUseLuminanceThreshold = false;

	UPROPERTY(EditAnywhere, Category="LXRFlux|Analyze", meta=(HideEditConditionToggle, EditCondition = "bUseLuminanceThreshold == true"))
	float LuminanceThreshold = 0.2f;

	UPROPERTY(EditAnywhere, Category="LXRFlux|Smoothing")
	TEnumAsByte<ESmoothMethod> SmoothMethod = ESmoothMethod::TargetValueOverTime;

	UPROPERTY(EditAnywhere, Category="LXRFlux|Smoothing", meta=(HideEditConditionToggle, EditCondition = "SmoothMethod == ESmoothMethod::TargetValueOverTime"))
	float TargetValueSmoothSpeed = 50.f;

	UPROPERTY(EditAnywhere, Category="LXRFlux|Smoothing", meta=(HideEditConditionToggle, EditCondition = "SmoothMethod == ESmoothMethod::HistoryBuffer"))
	int HistoryCount = 10;
	
	UPROPERTY(EditAnywhere, Category="LXRFlux|Smoothing", meta=(HideEditConditionToggle, EditCondition = "SmoothMethod == ESmoothMethod::ValidateDelta"))
	int RequiredStableFrames  = 3;

	UPROPERTY(EditAnywhere, Category="LXRFlux|Smoothing", meta=(HideEditConditionToggle, EditCondition = "SmoothMethod == ESmoothMethod::ValidateDelta"))
	float DeltaThreshold = 0.1f;
	
	UPROPERTY(EditAnywhere, Category="LXRFlux|SceneCapture")
	bool bWhitelistAllStaticMeshActors = true;

	UPROPERTY(EditAnywhere, Category="LXRFlux|SceneCapture")
	FString WhiteListTag;

	UPROPERTY(EditAnywhere, Category="LXRFlux|SceneCapture")
	float CaptureMeshScale = 1.f;

	UPROPERTY(EditAnywhere, Category="LXRFlux|SceneCapture")
	float TopCaptureWeight = 1.0f;

	UPROPERTY(EditAnywhere, Category="LXRFlux|SceneCapture")
	float BotCaptureWeight = 1.0f;

	// How frequently to perform lighting captures (in frames).
	// 1 = capture every frame (highest accuracy, highest cost)
	// 2 = capture every 2nd frame (50% less cost)
	// 3 = capture every 3rd frame, etc.
	// 
	// This controls how often the system performs a SceneCapture and GPU analysis.
	// Higher values reduce overhead at the cost of responsiveness.
	UPROPERTY(EditAnywhere, Category = "LXRFlux|SceneCapture", meta = (ClampMin = "1", ToolTip = "How frequently to perform lighting captures (in frames). 1 = every frame, 2 = every second frame, etc."))
	int CaptureRate = 1;



	/**
	* Logarithmic adjustment for the exposure.
	* 0: no adjustment, -1:2x darker, -2:4x darker, 1:2x brighter, 2:4x brighter, ...
	*/
	UPROPERTY(EditAnywhere, Category="LXRFlux|Detection")
	float AutoExposureBias = 8.f;

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

	UPROPERTY(BlueprintReadOnly, Category="LXRFlux|Final")
	float Luminance;

	UPROPERTY(BlueprintReadOnly, Category="LXRFlux|Final")
	float TopLuminance;

	UPROPERTY(BlueprintReadOnly, Category="LXRFlux|Final")
	float BotLuminance;

	UPROPERTY(BlueprintReadOnly, Category="LXRFlux|Final")
	FLinearColor Color;

	UPROPERTY(BlueprintReadOnly, Category="LXRFlux|Final")
	FLinearColor TopColor;

	UPROPERTY(BlueprintReadOnly, Category="LXRFlux|Final")
	FLinearColor BotColor;

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

	bool bNeedsSceneCaptureUpdate;

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
	float DeltaReadSeconds;
	float RequestCaptureTime;
	float ReadCompleteTime;
	float RemoveCaptureEveryFrameTimer;

	int SceneComponentDeltaChange = 1;
	int NewLumenCVarSum = 0;
	
	TArray<float*> LumenCVars;
	TArray<TObjectPtr<UTextureRenderTarget2D>> RenderTargets;

	FVector TargetLocation;
	TSharedPtr<FLXRFluxAnalyzeDispatchParams> CaptureAnalyzeDispatchParams;

	void AddNewSamplesToHistory();
	void OnReadbackComplete();
	float GetFinalLuminanceValue(EValueToGet ValueToGet);
	FLinearColor GetFinalColorValue(EValueToGet ValueToGet);
	

	TCircularHistoryBuffer<float> CombinedLuminanceHistory;
	TCircularHistoryBuffer<FLinearColor> CombinedColorHistory;

	TCircularHistoryBuffer<float> TopLuminanceHistory;
	TCircularHistoryBuffer<FLinearColor> TopColorHistory;

	TCircularHistoryBuffer<float> BotLuminanceHistory;
	TCircularHistoryBuffer<FLinearColor> BotColorHistory;

	TSharedPtr<FFluxOutput> FluxOutput;
	
	TUniquePtr<FLXRValidatedFloat> ValidatedLuminance;
	TUniquePtr<FLXRValidatedFloat> ValidatedTopLuminance;
	TUniquePtr<FLXRValidatedFloat> ValidatedBotLuminance;

	TUniquePtr<FLXRValidatedColor> ValidatedColor;
	TUniquePtr<FLXRValidatedColor> ValidatedTopColor;
	TUniquePtr<FLXRValidatedColor> ValidatedBotColor;
};
