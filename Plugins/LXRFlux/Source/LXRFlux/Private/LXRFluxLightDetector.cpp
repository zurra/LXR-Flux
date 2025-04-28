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

//Disclaimer//
/* This class is quite mess still, its copy from the LXR Indirect Detector and still needs to be cleaned up. */
//Disclaimer//

#include "LXRFluxLightDetector.h"

#include "LXRFlux.h"
#include "LXRFluxAnalyze.h"
#include "LXRFluxCaptureSubSystem.h"
#include "LXRFluxDebugCanvasWidget.h"
#include "LXRFLuxLightDetectorChildActor.h"
#include "LXRFluxRenderActorComponent.h"
#include "Blueprint/UserWidget.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/Material.h"
#include "Engine/StaticMesh.h"
#include "TimerManager.h"
#include "TextureResource.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Character.h"


static TAutoConsoleVariable<bool> CVarFLXRFluxIndirect(
	TEXT("FLXRFlux.disable"),
	0,
	TEXT("Should the FLXRFlux be disabled.\n")
	TEXT("  0: No\n")
	TEXT("  1: Yes\n"),
	ECVF_Scalability | ECVF_RenderThreadSafe);

static TAutoConsoleVariable<bool> CVarFLXRFluxCapture(
	TEXT("FLXRFlux.disable.capture"),
	0,
	TEXT("Should the FLXRFlux capture be disabled.\n")
	TEXT("  0: No\n")
	TEXT("  1: Yes\n"),
	ECVF_Scalability | ECVF_RenderThreadSafe);

static TAutoConsoleVariable<bool> CVarFLXRFluxIndirectDebugCapture(
	TEXT("FLXRFlux.debug.capture"),
	0,
	TEXT("Render the FLXRFlux Debug Widget.\n")
	TEXT("  0: Nope\n")
	TEXT("  1: Yep\n"),
	ECVF_Scalability | ECVF_RenderThreadSafe);

static TAutoConsoleVariable<bool> CVarFLXRFluxIndirectDebugMesh(
	TEXT("FLXRFlux.debug.mesh"),
	0,
	TEXT("Render the Mesh.\n")
	TEXT("  0: No thanks.\n")
	TEXT("  1: I need that!\n"),
	ECVF_Scalability | ECVF_RenderThreadSafe);

//
//Lumen


static TAutoConsoleVariable<float> CVarFLXRFluxIndirectLumenSceneLightingQuality(
	TEXT("FLXRFlux.Lumen.SceneLightingQuality"),
	1,
	TEXT("Scales Lumen Scene's quality \n")
	TEXT("Larger scales cause Lumen Scene to be calculated with a higher fidelity, which can be visible in reflections, but increase GPU cost.\n"),
	ECVF_Scalability | ECVF_RenderThreadSafe);

static TAutoConsoleVariable<float> CVarFLXRFluxIndirectLumenSceneDetail(
	TEXT("FLXRFlux.Lumen.SceneDetail"),
	1,
	TEXT("Controls the size of instances that can be represented in Lumen Scene. \n")
	TEXT("Larger values will ensure small objects are represented, but increase GPU cost.\n"),
	ECVF_Scalability | ECVF_RenderThreadSafe);

static TAutoConsoleVariable<float> CVarFLXRFluxIndirectLumenSceneViewDistance(
	TEXT("FLXRFlux.Lumen.SceneViewDistance"),
	5000,
	TEXT("Sets the maximum view distance of the scene that Lumen maintains for ray tracing against. \n")
	TEXT("Larger values will increase the effective range of sky shadowing and Global Illumination, but increase GPU cost.\n"),
	ECVF_Scalability | ECVF_RenderThreadSafe);

static TAutoConsoleVariable<float> CVarFLXRFluxIndirectLumenFinalGatherQuality(
	TEXT("FLXRFlux.Lumen.FinalGatherQuality"),
	1,
	TEXT("Scales Lumen's Final Gather quality.\n")
	TEXT("Larger scales reduce noise, but greatly increase GPU cost.\n"),
	ECVF_Scalability | ECVF_RenderThreadSafe);

static TAutoConsoleVariable<float> CVarFLXRFluxIndirectLumenMaxTraceDistance(
	TEXT("FLXRFlux.Lumen.MaxTraceDistance"),
	5000,
	TEXT("Controls the maximum distance that Lumen should trace while solving lighting.\n")
	TEXT("Values that are too small will cause lighting to leak into large caves, while values that are large will increase GPU cost.\n"),
	ECVF_Scalability | ECVF_RenderThreadSafe);

static TAutoConsoleVariable<float> CVarFLXRFluxIndirectLumenSurfaceCacheResolution(
	TEXT("FLXRFlux.Lumen.SurfaceCacheResolution"),
	0.5,
	TEXT("Scale factor for Lumen Surface Cache resolution, for Scene Capture.\n")
	TEXT("Smaller values save GPU memory, at a cost in quality.\n")
	TEXT("Defaults to 0.5 if not overridden.\n"),
	ECVF_Scalability | ECVF_RenderThreadSafe);

static TAutoConsoleVariable<float> CVarFLXRFluxIndirectLumenSceneLightingUpdateSpeed(
	TEXT("FLXRFlux.Lumen.SceneLightingUpdateSpeed"),
	1,
	TEXT("Controls how much Lumen Scene is allowed to cache lighting results to improve performance.\n")
	TEXT("Larger scales cause lighting changes to propagate faster, but increase GPU cost.\n"),
	ECVF_Scalability | ECVF_RenderThreadSafe);

static TAutoConsoleVariable<float> CVarFLXRFluxIndirectLumenFinalGatherLightingUpdateSpeed(
	TEXT("FLXRFlux.Lumen.FinalGatherLightingUpdateSpeed"),
	1,
	TEXT("Controls how much Lumen Final Gather is allowed to cache lighting results to improve performance.\n")
	TEXT("Larger scales cause lighting changes to propagate faster, but increase GPU cost and noise.\n"),
	ECVF_Scalability | ECVF_RenderThreadSafe);

static TAutoConsoleVariable<float> CVarFLXRFluxIndirectLumenDiffuseColorBoost(
	TEXT("FLXRFlux.Lumen.DiffuseColorBoost"),
	1,
	TEXT("Allows brightening indirect lighting by calculating material diffuse color for indirect lighting as pow(DiffuseColor, 1 / DiffuseColorBoost).\n")
	TEXT(
		"Values above 1 (original diffuse color) aren't physically correct, but they can be useful as an art direction knob to increase the amount of bounced light in the scene.\n")
	TEXT("Best to keep below 2 as it also causes reflections to be brighter than the scene.\n"),
	ECVF_Scalability | ECVF_RenderThreadSafe);

static TAutoConsoleVariable<float> CVarFLXRFluxIndirectLumenSkylightLeaking(
	TEXT("FLXRFlux.Lumen.SkylightLeaking"),
	0,
	TEXT("Controls what fraction of the skylight intensity should be allowed to leak.\n")
	TEXT("This can be useful as an art direction knob (non-physically based) to keep indoor areas from going fully black.\n"),
	ECVF_Scalability | ECVF_RenderThreadSafe);

static TAutoConsoleVariable<float> CVarFLXRFluxIndirectLumenFullSkylightLeakingDistance(
	TEXT("FLXRFlux.Lumen.FullSkylightLeakingDistance"),
	1000,
	TEXT("Controls the distance from a receiving surface where skylight leaking reaches its full intensity.\n")
	TEXT("Smaller values make the skylight leaking flatter, while larger values create an Ambient Occlusion effect.\n"),
	ECVF_Scalability | ECVF_RenderThreadSafe);


//


ULXRFluxLightDetectorComponent::ULXRFluxLightDetectorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	// SetComponentTickInterval(0.1f);
}


void ULXRFluxLightDetectorComponent::FetchLumenCVars(IConsoleVariable* Var)
{
	bFLXRFluxLumenSceneLightingQuality = CVarFLXRFluxIndirectLumenSceneLightingQuality.GetValueOnGameThread();
	bFLXRFluxLumenSceneDetail = CVarFLXRFluxIndirectLumenSceneDetail.GetValueOnGameThread();
	bFLXRFluxLumenSceneViewDistance = CVarFLXRFluxIndirectLumenSceneViewDistance.GetValueOnGameThread();
	bFLXRFluxLumenFinalGatherQuality = CVarFLXRFluxIndirectLumenFinalGatherQuality.GetValueOnGameThread();
	bFLXRFluxLumenMaxTraceDistance = CVarFLXRFluxIndirectLumenMaxTraceDistance.GetValueOnGameThread();
	bFLXRFluxLumenSurfaceCacheResolution = CVarFLXRFluxIndirectLumenSurfaceCacheResolution.GetValueOnGameThread();
	bFLXRFluxLumenSceneLightingUpdateSpeed = CVarFLXRFluxIndirectLumenSceneLightingUpdateSpeed.GetValueOnGameThread();
	bFLXRFluxLumenFinalGatherLightingUpdateSpeed = CVarFLXRFluxIndirectLumenFinalGatherLightingUpdateSpeed.GetValueOnGameThread();
	bFLXRFluxLumenDiffuseColorBoost = CVarFLXRFluxIndirectLumenDiffuseColorBoost.GetValueOnGameThread();
	bFLXRFluxLumenSkylightLeaking = CVarFLXRFluxIndirectLumenSkylightLeaking.GetValueOnGameThread();
	bFLXRFluxLumenFullSkylightLeakingDistance = CVarFLXRFluxIndirectLumenFullSkylightLeakingDistance.GetValueOnGameThread();
	bFLXRFluxDisabled = CVarFLXRFluxIndirect.GetValueOnAnyThread();
	bFLXRFluxDebugCaptureWidgetEnabled = CVarFLXRFluxIndirectDebugCapture.GetValueOnGameThread();
	bFLXRFluxDebugMeshEnabled = CVarFLXRFluxIndirectDebugMesh.GetValueOnAnyThread();

	bFLXRFluxCaptureDisabled = bFLXRFluxDisabled ? 1 : CVarFLXRFluxCapture.GetValueOnGameThread();

	for (int l = 0; l < LumenCVars.Max(); ++l)
	{
		NewLumenCVarSum += *LumenCVars[l];
	}

	if (NewLumenCVarSum != LumenCVarsSum)
	{
		LumenCVarsSum = NewLumenCVarSum;
		UpdatePostprocessingLumenSettings();
	}

	IndirectMeshComponent->SetVisibleInSceneCaptureOnly(!bFLXRFluxDebugMeshEnabled);


	if (bFLXRFluxDebugCaptureWidgetEnabled)
	{
		if (!IndirectDebugWidget.IsValid())
		{
			ULXRFluxSubSystem* SubSystem = GetWorld()->GetGameInstance()->GetSubsystem<ULXRFluxSubSystem>();
			FString AssetPath = SubSystem->GetLXRAssetPath();
			FString WidgetPath = AssetPath + "/Widget/WB_LxrCanvas.WB_LxrCanvas_C";
			const FSoftClassPath SoftCanvasPath(WidgetPath);

			TSubclassOf<UUserWidget> WidgetClass = SoftCanvasPath.TryLoadClass<UUserWidget>();

			if (WidgetClass)
			{
				UUserWidget* Widget = CreateWidget<UUserWidget>(GetWorld(), WidgetClass);
				if (Widget)
				{
					IndirectDebugWidget = Widget;
				}
			}
		}
		if (IndirectDebugWidget.IsValid())
		{
			IndirectDebugWidget->AddToViewport();
			ULXRFluxDebugCanvasWidget* DebugCanvas = Cast<ULXRFluxDebugCanvasWidget>(IndirectDebugWidget);
			DebugCanvas->SetDebugActor(GetOwner());
		}
	}
	else
	{
		if (IndirectDebugWidget.IsValid())
		{
			IndirectDebugWidget->RemoveFromParent();
		}
	}
}

void ULXRFluxLightDetectorComponent::BindCvarDelegates()
{
	FConsoleVariableDelegate CheckLumenCvarValues = FConsoleVariableDelegate::CreateUObject(this, &ULXRFluxLightDetectorComponent::FetchLumenCVars);

	CVarFLXRFluxIndirectLumenSceneLightingQuality->SetOnChangedCallback(CheckLumenCvarValues);
	CVarFLXRFluxIndirectLumenSceneDetail->SetOnChangedCallback(CheckLumenCvarValues);
	CVarFLXRFluxIndirectLumenSceneViewDistance->SetOnChangedCallback(CheckLumenCvarValues);
	CVarFLXRFluxIndirectLumenFinalGatherQuality->SetOnChangedCallback(CheckLumenCvarValues);
	CVarFLXRFluxIndirectLumenMaxTraceDistance->SetOnChangedCallback(CheckLumenCvarValues);
	CVarFLXRFluxIndirectLumenSurfaceCacheResolution->SetOnChangedCallback(CheckLumenCvarValues);
	CVarFLXRFluxIndirectLumenSceneLightingUpdateSpeed->SetOnChangedCallback(CheckLumenCvarValues);
	CVarFLXRFluxIndirectLumenFinalGatherLightingUpdateSpeed->SetOnChangedCallback(CheckLumenCvarValues);
	CVarFLXRFluxIndirectLumenDiffuseColorBoost->SetOnChangedCallback(CheckLumenCvarValues);
	CVarFLXRFluxIndirectLumenSkylightLeaking->SetOnChangedCallback(CheckLumenCvarValues);
	CVarFLXRFluxIndirectLumenFullSkylightLeakingDistance->SetOnChangedCallback(CheckLumenCvarValues);
	CVarFLXRFluxCapture->SetOnChangedCallback(CheckLumenCvarValues);
	CVarFLXRFluxIndirectDebugCapture->SetOnChangedCallback(CheckLumenCvarValues);
	CVarFLXRFluxIndirectDebugMesh->SetOnChangedCallback(CheckLumenCvarValues);
}

// Called when the game starts
void ULXRFluxLightDetectorComponent::BeginPlay()
{
	Super::BeginPlay();

	LumenCVars = {
		&bFLXRFluxLumenSceneLightingQuality,
		&bFLXRFluxLumenSceneLightingQuality,
		&bFLXRFluxLumenSceneDetail,
		&bFLXRFluxLumenSceneViewDistance,
		&bFLXRFluxLumenFinalGatherQuality,
		&bFLXRFluxLumenMaxTraceDistance,
		&bFLXRFluxLumenSurfaceCacheResolution,
		&bFLXRFluxLumenSceneLightingUpdateSpeed,
		&bFLXRFluxLumenFinalGatherLightingUpdateSpeed,
		&bFLXRFluxLumenDiffuseColorBoost,
		&bFLXRFluxLumenSkylightLeaking,
		&bFLXRFluxLumenFullSkylightLeakingDistance
	};

	PrepareChildActor();

	if (ChildActorComponent == NULL)
	{
		UE_LOG(LogLXRFlux, Error, TEXT("%s Child Actor missing, please add it."), *GetOwner()->GetName());
		UE_LOG(LogLXRFlux, Error, TEXT("%s Disabling FLXRFlux Indirect."), *GetOwner()->GetName());

		PrimaryComponentTick.SetTickFunctionEnable(false);
		return;
	}
	CreateRenderTargets();
	CreateCapturePrerequisites();

	// TArray<FRenderTarget*> Rts = {TopRT->GameThread_GetRenderTargetResource(),BotRT->GameThread_GetRenderTargetResource()};
	for (const auto& SceneCapture : SceneCaptures)
	{
		UE_LOG(LogLXRFlux, Log, TEXT("%s Found created scenecapture: %s"), *GetOwner()->GetName(), *SceneCapture->GetName());
	}


	BindCvarDelegates();
	FetchLumenCVars(NULL);

	for (int l = 0; l < LumenCVars.Max(); ++l)
	{
		LumenCVarsSum += *LumenCVars[l];
	}

	IndirectMeshComponent->SetRelativeLocation(FVector::UpVector);
	PrimaryComponentTick.SetTickFunctionEnable(true);

	TargetLocation = FVector::UpVector * 1000;

	IndirectMeshComponent->SetVisibleInSceneCaptureOnly(!bFLXRFluxDebugMeshEnabled);


	CombinedLuminanceHistory = TCircularHistoryBuffer<float>(HistoryCount);
	CombinedColorHistory = TCircularHistoryBuffer<FLinearColor>(HistoryCount);

	TopLuminanceHistory = TCircularHistoryBuffer<float>(HistoryCount);
	TopColorHistory = TCircularHistoryBuffer<FLinearColor>(HistoryCount);

	BotLuminanceHistory = TCircularHistoryBuffer<float>(HistoryCount);
	BotColorHistory = TCircularHistoryBuffer<FLinearColor>(HistoryCount);

	ValidatedLuminance = MakeUnique<FLXRValidatedFloat>(RequiredStableFrames, DeltaThreshold);
	ValidatedTopLuminance = MakeUnique<FLXRValidatedFloat>(RequiredStableFrames, DeltaThreshold);
	ValidatedBotLuminance = MakeUnique<FLXRValidatedFloat>(RequiredStableFrames, DeltaThreshold);

	ValidatedColor = MakeUnique<FLXRValidatedColor>(RequiredStableFrames, DeltaThreshold);
	ValidatedTopColor = MakeUnique<FLXRValidatedColor>(RequiredStableFrames, DeltaThreshold);
	ValidatedBotColor = MakeUnique<FLXRValidatedColor>(RequiredStableFrames, DeltaThreshold);

	FluxOutput = MakeShared<FFluxOutput>();
	CaptureAnalyzeDispatchParams = MakeShared<FLXRFluxAnalyzeDispatchParams>();

	CaptureAnalyzeDispatchParams->FrameCaptureMax = FMath::Max(CaptureRate, 1);
	CaptureAnalyzeDispatchParams->Output = FluxOutput;
	CaptureAnalyzeDispatchParams->IndirectDetector = this;
	CaptureAnalyzeDispatchParams->LuminanceThreshold = bUseLuminanceThreshold ? LuminanceThreshold : 0.0f;;
	CaptureAnalyzeDispatchParams->RenderTargetTop = CaptureAnalyzeDispatchParams->IndirectDetector->GetTopTarget()->GameThread_GetRenderTargetResource();
	CaptureAnalyzeDispatchParams->IndirectDetector->GetTopTarget()->UpdateResourceImmediate();
	CaptureAnalyzeDispatchParams->RenderTargetBot = CaptureAnalyzeDispatchParams->IndirectDetector->GetBotTarget()->GameThread_GetRenderTargetResource();
	CaptureAnalyzeDispatchParams->IndirectDetector->GetBotTarget()->UpdateResourceImmediate();
	CaptureAnalyzeDispatchParams->OnReadbackComplete = [this]() { this->OnReadbackComplete(); };

	CaptureAnalyzeDispatchParams->bIsInitialized = true;


	//Lazy "late begin play"
	FTimerHandle Temp;
	GetWorld()->GetTimerManager().SetTimer(Temp, FTimerDelegate::CreateLambda([&]
	{
		RequestOneShotCaptureUpdate();
	}), 0.1f, false);
}

void ULXRFluxLightDetectorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (CaptureAnalyzeDispatchParams.IsValid())
	{
		CaptureAnalyzeDispatchParams->OnReadbackComplete.Reset();
		CaptureAnalyzeDispatchParams.Reset();
	}
	Super::EndPlay(EndPlayReason);
}

// Called every frame
void ULXRFluxLightDetectorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	if (ChildActorComponent && ChildActorComponent->GetChildActor())
	{
		AActor* Child = ChildActorComponent->GetChildActor();
		Child->GetRootComponent()->SetWorldRotation(FRotator::ZeroRotator);
		Child->GetRootComponent()->SetWorldScale3D(FVector(CaptureMeshScale));
		// Nudge the SceneCaptures because of reasons... (Lumen does not want to update if SceneCapture does not move)
		TopSceneCaptureComponent->AddRelativeLocation(FVector::UpVector * SceneComponentDeltaChange);
		BotSceneCaptureComponent->AddRelativeLocation(FVector::UpVector * SceneComponentDeltaChange);
		IndirectMeshComponent->AddRelativeLocation(FVector::DownVector * SceneComponentDeltaChange);
		SceneComponentDeltaChange *= -1;
	}

	if (bFLXRFluxDisabled)
	{
		GEngine->AddOnScreenDebugMessage(static_cast<int32>(GetUniqueID()), DeltaTime, FColor::Red, FString::Printf(TEXT("FLXRFlux Indirect Disabled")));
		return;
	}
	else if (bFLXRFluxCaptureDisabled)
	{
		TArray<FString> DisabledArray;
		FString Disabled;
		if (bFLXRFluxCaptureDisabled)
			DisabledArray.Add("Capture");

		if (DisabledArray.Num() == 1)
		{
			Disabled = DisabledArray[0];
		}
		else
		{
			for (auto String : DisabledArray)
			{
				Disabled += String + ", ";
			}
			Disabled.LeftChopInline(2);
		}
		GEngine->AddOnScreenDebugMessage(static_cast<int32>(GetUniqueID()), DeltaTime, FColor::Red, FString::Printf(TEXT("FLXRFlux Indirect %s Disabled"), *Disabled));
	}


	// if (bFLXRFluxDebugCaptureWidgetEnabled)
	// {
	// 	for (auto& CaptureComponent2D : SceneCaptures)
	// 	{
	// 		DrawDebugCamera(GetWorld(), CaptureComponent2D->GetComponentLocation(), CaptureComponent2D->GetComponentRotation(), CaptureComponent2D->FOVAngle, 4, FColor::Green, 0,
	// 		                DeltaTime);
	//
	// 		ChildActorComponent->GetChildActor()->SetActorLocation(GetOwner()->GetActorLocation());
	// 		ChildActorComponent->SetWorldLocation(GetOwner()->GetActorLocation());
	//
	// 		DrawDebugDirectionalArrow(GetWorld(), GetOwner()->GetActorLocation(), ChildActorComponent.Get()->GetChildActor()->GetActorLocation(), 250.f, FColor::Red);
	// 	}
	// }

	// Luminance = FMath::FInterpConstantTo(Luminance, LuminanceTarget, DeltaTime, 2);
	// ColorOutput = FMath::CInterpTo(ColorOutput, ColorOutputTarget, DeltaTime, 2);
	// IndirectMeshComponent->SetWorldRotation(FQuat::MakeFromEuler(FVector::RightVector));
	// TopSceneCaptureComponent->SetRelativeTransform(FTransform(FQuat::MakeFromEuler(FVector(r ? 0 : 0, r ? 90 : -90, 45.f)), FVector(0, 0, r ? -120 : 120)));
	// BotSceneCaptureComponent->SetRelativeTransform(FTransform(FQuat::MakeFromEuler(FVector(r ? 0 : 0, r ? -90 : 90, 45.f)), FVector(0, 0, r ? 120 : -120)));

	// TargetChangeTimer += DeltaTime;

	// if (TargetChangeTimer > 0.03f)
	// {
	// 	// TargetChangeTimer = 0;
	// 	TargetLocation = TargetLocation * (FVector::UpVector * -1);
	// 	IndirectMeshComponent->SetRelativeTransform(FTransform(FQuat::MakeFromEuler(FVector(r ? 0 : 1, r ? 0 : 1, r ? 0 : 1)), IndirectMeshComponent->GetRelativeLocation(),
	// 	                                                       FVector(1, 1, 1)));
	// 	// auto x = IndirectMeshComponent->GetRelativeRotation().Euler();
	// 	// auto V = FVector::RightVector;
	// 	// V=V*45;
	// 	// IndirectMeshComponent->AddWorldRotation(FRotator::MakeFromEuler(V));
	//
	// 	TopSceneCaptureComponent->SetRelativeTransform(FTransform(FQuat::MakeFromEuler(FVector(0, -90, 45.f)), FVector(0, 0, r ? 75 : 70)));
	// 	BotSceneCaptureComponent->SetRelativeTransform(FTransform(FQuat::MakeFromEuler(FVector(0, 90, 45.f)), FVector(0, 0, r ? -75 : -70)));
	// 	r = !r;
	// }


	// Luminance = GetBrightness();
	// ColorOutput = GetColor();

	// TargetChangeTimer += DeltaTime;
	//
	// if (TargetChangeTimer > 0.025)
	// {
	// 	for (auto SceneCapture : SceneCaptures)
	// 	{
	// 		TargetChangeTimer = 0;
	// 		// SceneCapture->bCaptureEveryFrame = true;
	// 		SceneCapture->CaptureSceneDeferred();
	// 	}
	// }

	// int32 FrameNumber = GFrameNumber;
	//
	// UE_LOG(LogTemp, Warning, TEXT("[FLXRFlux - Tick:%d ]"), FrameNumber);
	//
	// if (IsReadbackReady(IndirectAnalyzeDispatchParams))
	// {
	// 	IndirectAnalyzeDispatchParams->bReadingInProgress;
	// 	IndirectAnalyzeDispatchParams->ReadbackReadyFrames++;
	//
	// 	UE_LOG(LogTemp, Warning, TEXT("[FLXRFlux] Waiting... Frames: %d"), IndirectAnalyzeDispatchParams->ReadbackReadyFrames);
	//
	// 	if (IndirectAnalyzeDispatchParams->ReadbackReadyFrames > 5)
	// 	{
	// 		UE_LOG(LogTemp, Warning, TEXT("Readback ready, dispatching GPU read..."));
	//
	// 		TriggerReadback(IndirectAnalyzeDispatchParams);
	// 	}
	// }

	HandleOneShotSceneCaptureCooldown();

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}


float ULXRFluxLightDetectorComponent::GetFinalLuminanceValue(EValueToGet ValueToGet)
{
	float Current = 0.0f;
	float Target = 0.0f;
	const TCircularHistoryBuffer<float>* HistoryBufferPtr = nullptr;
	float SmoothSpeed = TargetValueSmoothSpeed;
	FLXRValidatedFloat* ValidatedFloatPtr = nullptr;

	switch (ValueToGet)
	{
	case Top:
		Current = TopLuminance;
		Target = FluxOutput->TopLuminance;
		HistoryBufferPtr = &TopLuminanceHistory;
		ValidatedFloatPtr = ValidatedTopLuminance.Get();
		break;

	case Bot:
		Current = BotLuminance;
		Target = FluxOutput->BotLuminance;
		HistoryBufferPtr = &BotLuminanceHistory;
		ValidatedFloatPtr = ValidatedBotLuminance.Get();
		break;

	case Combined:
		Current = Luminance;
		Target = FluxOutput->Luminance;
		HistoryBufferPtr = &CombinedLuminanceHistory;
		ValidatedFloatPtr = ValidatedLuminance.Get();
		break;
	}

	if (Target < Current)
		SmoothSpeed = 1000;

	switch (SmoothMethod)
	{
	case TargetValueOverTime:
		return FMath::FInterpConstantTo(Current, Target, DeltaReadSeconds, SmoothSpeed);

	case HistoryBuffer:
		if (HistoryBufferPtr)
		{
			float Sum = 0.0f;
			int32 Count = HistoryBufferPtr->Num();

			for (int32 Index = 0; Index < Count; ++Index)
			{
				Sum += (*HistoryBufferPtr)[Index];
			}
			return (Count > 0) ? Sum / Count : 0.0f;
		}
		break;

	case None:
		return Target;
	case ValidateDelta:
		return ValidatedFloatPtr->Tick(Target);
	default: ;
	}

	// Fallback (shouldn't happen, but just in case)
	return 0.0f;
}


FLinearColor ULXRFluxLightDetectorComponent::GetFinalColorValue(EValueToGet ValueToGet)
{
	const FLinearColor* Current = nullptr;
	const FLinearColor* Target = nullptr;
	const TCircularHistoryBuffer<FLinearColor>* HistoryBufferPtr = nullptr;
	float SmoothSpeed = TargetValueSmoothSpeed;
	FLXRValidatedColor* ValidatedColorPtr = nullptr;

	switch (ValueToGet)
	{
	case Top:
		Current = &TopColor;
		Target = &FluxOutput->TopColor;
		HistoryBufferPtr = &TopColorHistory;
		ValidatedColorPtr = ValidatedTopColor.Get();
		break;

	case Bot:
		Current = &BotColor;
		Target = &FluxOutput->BotColor;
		HistoryBufferPtr = &BotColorHistory;
		ValidatedColorPtr = ValidatedBotColor.Get();
		break;

	case Combined:
		Current = &Color;
		Target = &FluxOutput->Color;
		HistoryBufferPtr = &CombinedColorHistory;
		ValidatedColorPtr = ValidatedColor.Get();
		break;
	}

	if (Target->GetLuminance() < Current->GetLuminance())
		SmoothSpeed = 1000;

	switch (SmoothMethod)
	{
	case TargetValueOverTime:
		return FMath::CInterpTo(*Current, *Target, DeltaReadSeconds, SmoothSpeed);


	case HistoryBuffer:
		if (HistoryBufferPtr)
		{
			FLinearColor Sum = FLinearColor::Black;
			int32 Count = HistoryBufferPtr->Num();


			for (int32 Index = 0; Index < Count; ++Index)
			{
				Sum += (*HistoryBufferPtr)[Index];
			}
			return (Count > 0) ? Sum / Count : FLinearColor::Black;
		}
		break;

	case None:
		return *Target;
	case ValidateDelta:
		if (ValidatedColorPtr)
		{
			return ValidatedColorPtr->Tick(*Target);
		}
		break;
	default: ;
	}

	// Fallback (shouldn't happen, but just in case)
	return FLinearColor::Black;
}

void ULXRFluxLightDetectorComponent::AddNewSamplesToHistory()
{
	ELXRFluxCaptureStep CurrentStep = static_cast<ELXRFluxCaptureStep>(CaptureAnalyzeDispatchParams->CaptureStepCounter);

	switch (CurrentStep)
	{
	case ELXRFluxCaptureStep::Top:
		TopLuminanceHistory.Add(FluxOutput->TopLuminance);
		TopColorHistory.Add(FluxOutput->TopColor);
		break;
	case ELXRFluxCaptureStep::Bot:
		BotLuminanceHistory.Add(FluxOutput->BotLuminance);
		BotColorHistory.Add(FluxOutput->BotColor);
		break;
	case ELXRFluxCaptureStep::Wait:
		CombinedLuminanceHistory.Add(FluxOutput->Luminance);
		CombinedColorHistory.Add(FluxOutput->Color);
		break;
	}
}


void ULXRFluxLightDetectorComponent::OnReadbackComplete()
{
	ReadCompleteTime = GetWorld()->GetTime().GetRealTimeSeconds();
	DeltaReadSeconds = ReadCompleteTime - RequestCaptureTime;

	if (static_cast<ELXRFluxCaptureStep>(CaptureAnalyzeDispatchParams->CaptureStepCounter) == ELXRFluxCaptureStep::Wait)
	{
		float TotalWeight = TopCaptureWeight + BotCaptureWeight;
		TotalWeight = FMath::Max(TotalWeight, 1);

		FluxOutput->Color = (FluxOutput->TopColor * TopCaptureWeight + FluxOutput->BotColor * BotCaptureWeight) / TotalWeight;
		FluxOutput->Luminance = (FluxOutput->TopLuminance * TopCaptureWeight + FluxOutput->BotLuminance * BotCaptureWeight) / TotalWeight;
	}

	if (SmoothMethod == HistoryBuffer)
	{
		AddNewSamplesToHistory();
	}

	TopLuminance = GetFinalLuminanceValue(EValueToGet::Top);
	TopColor = GetFinalColorValue(EValueToGet::Top);

	Luminance = GetFinalLuminanceValue(Combined);
	Color = GetFinalColorValue(Combined);

	BotLuminance = GetFinalLuminanceValue(EValueToGet::Bot);
	BotColor = GetFinalColorValue(EValueToGet::Bot);

	// UE_LOG(LogTemp, Log, TEXT("[FLXRFlux] TopLuminance: %.4f"), TopLuminance);
	// UE_LOG(LogTemp, Log, TEXT("[FLXRFlux] Top RGB: R=%.4f G=%.4f B=%.4f"), TopColor.R, TopColor.G, TopColor.B);
	//
	// UE_LOG(LogTemp, Log, TEXT("[FLXRFlux] Luminance: %.4f"), Luminance);
	// UE_LOG(LogTemp, Log, TEXT("[FLXRFlux] RGB: R=%.4f G=%.4f B=%.4f"), Color.R, Color.G, Color.B);
	//
	// UE_LOG(LogTemp, Log, TEXT("[FLXRFlux] Bot Luminance: %.4f"), BotLuminance);
	// UE_LOG(LogTemp, Log, TEXT("[FLXRFlux] Bot RGB: R=%.4f G=%.4f B=%.4f"), BotColor.R, BotColor.G, BotColor.B);
	RequestOneShotCaptureUpdate();
}


void ULXRFluxLightDetectorComponent::RequestOneShotCaptureUpdate()
{
	RemoveCaptureEveryFrameTimer = 0;
	bNeedsSceneCaptureUpdate = true;

	if (!CaptureAnalyzeDispatchParams.IsValid()) return;
	CaptureAnalyzeDispatchParams->IncreaseCaptureCounter();

	ELXRFluxCaptureStep CurrentStep = static_cast<ELXRFluxCaptureStep>(CaptureAnalyzeDispatchParams->CaptureStepCounter);

	if (CaptureAnalyzeDispatchParams->FrameCounter == 0)
	{
		switch (CurrentStep)
		{
		case ELXRFluxCaptureStep::Top:
		case ELXRFluxCaptureStep::Bot:
			{
				const int32 CaptureIndex = static_cast<int32>(CurrentStep);

				if (SceneCaptures.IsValidIndex(CaptureIndex))
				{
					if (USceneCaptureComponent2D* Capture = SceneCaptures[CaptureIndex])
					{
						Capture->CaptureSceneDeferred();
					}
				}

				CaptureAnalyzeDispatchParams->bReadingInProgress = false;

				if (ULXRFluxSubSystem* SubSystem = GetWorld()->GetGameInstance()->GetSubsystem<ULXRFluxSubSystem>())
				{
					SubSystem->RequestIndirectAnalyze(CaptureAnalyzeDispatchParams);
					RequestCaptureTime = GetWorld()->GetTime().GetRealTimeSeconds();
				}
				break;
			}

		case ELXRFluxCaptureStep::Wait:
		default:
			OnReadbackComplete();
			break;
		}
	}
	else
	{
		RequestOneShotCaptureUpdate();
	}
}

void ULXRFluxLightDetectorComponent::HandleOneShotSceneCaptureCooldown()
{
	if (bNeedsSceneCaptureUpdate && ++RemoveCaptureEveryFrameTimer > 2)
	{
		for (auto SceneCapture : SceneCaptures)
		{
			if (SceneCapture)
			{
				SceneCapture->bCaptureEveryFrame = false;
			}
		}
		bNeedsSceneCaptureUpdate = false;
	}
}

void ULXRFluxLightDetectorComponent::CreateRenderTargets()
{
	int W = RenderTextureSize;
	int H = RenderTextureSize;

	if (TopRT == nullptr)
		TopRT = NewObject<UTextureRenderTarget2D>();

	RenderTargets.Add(TopRT);

	if (BotRT == nullptr)
		BotRT = NewObject<UTextureRenderTarget2D>();

	RenderTargets.Add(BotRT);


	for (auto& RT : RenderTargets)
	{
		RT->InitCustomFormat(W, H, PF_FloatRGB, true);
		RT->RenderTargetFormat = RTF_RGBA32f;
		RT->bGPUSharedFlag = true;
		RT->UpdateResourceImmediate();
	}

	TopRT->ClearColor = FLinearColor::Red;
	BotRT->ClearColor = FLinearColor::Green;


	UE_LOG(LogLXRFlux, Log, TEXT("%s Created RenderTargets"), *GetOwner()->GetName());
}

void ULXRFluxLightDetectorComponent::CreateCapturePrerequisites()
{
	TArray<FTransform> CaptureTransforms;

	const FTransform TopTransform = FTransform(FQuat::MakeFromEuler(FVector(0, -90, 45)), FVector(0, 0, 75));
	const FTransform BotTransform = FTransform(FQuat::MakeFromEuler(FVector(0, 90, 45)), FVector(0, 0, -75));
	CaptureTransforms.Add(TopTransform);
	CaptureTransforms.Add(BotTransform);


	int index = 0;
	for (auto& CaptureTransform : CaptureTransforms)
	{
		if (index == 0)
		{
			TopSceneCaptureComponent = Cast<USceneCaptureComponent2D>(
				ChildActorComponent->GetChildActor()->AddComponentByClass(USceneCaptureComponent2D::StaticClass(), true, {}, true));
			TopSceneCaptureComponent->SetupAttachment(ChildActorComponent->GetChildActor()->GetRootComponent());
			TopSceneCaptureComponent->SetRelativeTransform(CaptureTransforms[index]);
		}
		else
		{
			BotSceneCaptureComponent = Cast<USceneCaptureComponent2D>(
				ChildActorComponent->GetChildActor()->AddComponentByClass(USceneCaptureComponent2D::StaticClass(), true, {}, true));
			BotSceneCaptureComponent->SetupAttachment(ChildActorComponent->GetChildActor()->GetRootComponent());
			BotSceneCaptureComponent->SetRelativeTransform(CaptureTransforms[index]);
		}
		index++;
	}

	ChildActorComponent->GetChildActor()->GetComponents<USceneCaptureComponent2D>(SceneCaptures);


	// TArray<FString> ShowFlags = {"Antialiasing", "Atmosphere", "bsp", "decals", "fog", "landscape", "particles", "skeletalmeshes", "translucency", "instancedfoliage", "instancedgrass", "instancedstaticmeshes", "nanitemeshes", "paper2dsprites", "textrender", "temporalaa", "bloom", "eyeAdaptation", "LocalExposure", "motionblur", "tonecurve", "ambientocclusion", "distancefieldao", "reflectionenvironment", "ScreenSpaceReflections", "volumetricfog"};
	// for (const auto& CaptureComponent : SceneCaptures)
	// {
	// 	for (const FString& ShowFlag : ShowFlags)
	// 	{
	// 		FEngineShowFlagsSetting setting = FEngineShowFlagsSetting({ShowFlag, false});
	// 		CaptureComponent->ShowFlagSettings.Add(setting);
	// 	}
	// }

	ULXRFluxSubSystem* SubSystem = GetWorld()->GetGameInstance()->GetSubsystem<ULXRFluxSubSystem>();
	FString AssetPath = SubSystem->GetLXRAssetPath();
	FString MeshPath = AssetPath + "/Assets/LXRIndirectMeshV2.LXRIndirectMeshV2";
	FString MaterialPath = AssetPath + "/Assets/LXRIndirectMeshMat.LXRIndirectMeshMat";
	UStaticMesh* IndirectMesh = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), NULL, *MeshPath));
	UMaterial* IndirectMeshMat = Cast<UMaterial>(StaticLoadObject(UMaterial::StaticClass(), nullptr, *MaterialPath));


	IndirectMeshComponent = Cast<UStaticMeshComponent>(ChildActorComponent->GetChildActor()->GetComponentByClass(UStaticMeshComponent::StaticClass()));
	IndirectMatInst = UKismetMaterialLibrary::CreateDynamicMaterialInstance(GetWorld(), IndirectMeshMat, "IndirectMatInst");
	// checkf(!IndirectMatInst, TEXT("We failed to load the Indirect Material Instance.....!"));

	IndirectMatInst->SetScalarParameterValue("SingleCapture", 0);
	// IndirectMeshComponent->SetupAttachment(ChildActorComponent.Get()->GetChildActor()->GetRootComponent());
	IndirectMeshComponent->SetRelativeTransform(FTransform::Identity);

	IndirectMeshComponent->SetStaticMesh(IndirectMesh);
	IndirectMeshComponent->SetMaterial(0, IndirectMatInst);
	IndirectMeshComponent->SetAffectDynamicIndirectLighting(false);
	IndirectMeshComponent->SetAffectDistanceFieldLighting(false);
	IndirectMeshComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	IndirectMeshComponent->SetVisibleInSceneCaptureOnly(!bFLXRFluxDebugMeshEnabled);

	if (!bCaptureDirect)
	{
		IndirectMeshComponent->LightingChannels.bChannel0 = false;
		IndirectMeshComponent->LightingChannels.bChannel1 = false;
		IndirectMeshComponent->LightingChannels.bChannel2 = false;
	}
	IndirectMeshComponent->CastShadow = false;
	IndirectMeshComponent->bUseAsOccluder = false;
	IndirectMeshComponent->bVisibleInReflectionCaptures = false;
	IndirectMeshComponent->bVisibleInRealTimeSkyCaptures = false;
	IndirectMeshComponent->bReceivesDecals = false;

	FPostProcessSettings PostProcessSettings;
	PostProcessSettings.bOverride_DynamicGlobalIlluminationMethod = true;
	PostProcessSettings.DynamicGlobalIlluminationMethod = bCaptureIndirect ? EDynamicGlobalIlluminationMethod::Lumen : EDynamicGlobalIlluminationMethod::None;

	PostProcessSettings.AddBlendable(GetWorld()->GetGameInstance()->GetSubsystem<ULXRFluxSubSystem>()->IndirectPostProcessMaterial, 1);

	PostProcessSettings.bOverride_ReflectionMethod = true;
	PostProcessSettings.ReflectionMethod = bCaptureIndirect ? EReflectionMethod::Lumen : EReflectionMethod::None;

	// PostProcessSettings.bOverride_LumenSceneLightingQuality = true;
	// PostProcessSettings.bOverride_LumenSceneDetail = true;
	// PostProcessSettings.bOverride_LumenSceneViewDistance = true;
	// PostProcessSettings.bOverride_LumenFinalGatherQuality = true;
	// PostProcessSettings.bOverride_LumenMaxTraceDistance = true;
	// PostProcessSettings.bOverride_LumenSurfaceCacheResolution = true;
	// PostProcessSettings.bOverride_LumenSceneLightingUpdateSpeed = true;
	// PostProcessSettings.bOverride_LumenFinalGatherLightingUpdateSpeed = true;
	// PostProcessSettings.bOverride_LumenDiffuseColorBoost = true;
	// PostProcessSettings.bOverride_LumenSkylightLeaking = true;
	// PostProcessSettings.bOverride_LumenFullSkylightLeakingDistance = true;
	// PostProcessSettings.LumenMaxTraceDistance = 200;

	TopSceneCaptureComponent->TextureTarget = TopRT;
	BotSceneCaptureComponent->TextureTarget = BotRT;

	for (const auto& SceneCapture : SceneCaptures)
	{
		SceneCapture->bCaptureEveryFrame = false;
		SceneCapture->bCaptureOnMovement = false;
		SceneCapture->bConsiderUnrenderedOpaquePixelAsFullyTranslucent = true;

		SceneCapture->CaptureSource = ESceneCaptureSource::SCS_SceneColorHDR;
		SceneCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;


		// Todo move this all to subsystem.
		TArray<AActor*> ActorsToRender;
		for (TObjectIterator<ULXRFluxRenderActorComponent> Itr; Itr; ++Itr)
		{
			AActor* Actor = Itr->GetOwner();
			if (Actor != nullptr)
			{
				ActorsToRender.Add(Actor);
			}
		}

		for (TObjectIterator<AActor> Itr; Itr; ++Itr)
		{
			AActor* Actor = *Itr;
			if (Actor->ActorHasTag(*WhiteListTag))
			{
				ActorsToRender.Add(Actor);
			}
		}

		if (bWhitelistAllStaticMeshActors)
		{
			for (TObjectIterator<AStaticMeshActor> Itr; Itr; ++Itr)
			{
				ActorsToRender.Add(*Itr);
			}
		}

		for (auto ToRender : ActorsToRender)
		{
			SceneCapture->ShowOnlyActors.Append(ActorsToRender);
		}

		SceneCapture->ShowOnlyActorComponents(ChildActorComponent.Get()->GetChildActor());

		SceneCapture->FOVAngle = 35;
		SceneCapture->TextureTarget->SizeX = RenderTextureSize;
		SceneCapture->TextureTarget->SizeY = RenderTextureSize;


		SceneCapture->TextureTarget->RenderTargetFormat = RTF_RGBA32f;
		SceneCapture->TextureTarget->ClearColor = FColor::Black;
		SceneCapture->PostProcessSettings = PostProcessSettings;
		SceneCapture->bAlwaysPersistRenderingState = true;

		SceneCapture->ShowFlags.SetMaterials(false);

		// SceneCapture->TextureTarget->RenderTargetFormat = RTF_R32f;

		SceneCapture->ShowFlags.SetTonemapper(false);
		SceneCapture->ShowFlags.SetEyeAdaptation(false);

		SceneCapture->PostProcessSettings.bOverride_AutoExposureMethod = true;
		SceneCapture->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
		SceneCapture->PostProcessSettings.bOverride_AutoExposureBias = true;
		SceneCapture->PostProcessSettings.AutoExposureBias = AutoExposureBias;

		// ScreenPercentage fix.
		SceneCapture->ShowFlags.ScreenPercentage = true;
		SceneCapture->LODDistanceFactor = 1.0f; // optional, avoid streaming bias
		SceneCapture->PostProcessSettings.ScreenPercentage_DEPRECATED = 88;
		// r.SceneRenderTargetResizeMethod=2 ; // Prevent dynamic downscale
		// r.ScreenPercentage=100 ; // Force full-res if needed


		SceneCapture->TextureTarget->UpdateResourceImmediate();
	}


	for (const auto& CaptureComponent : SceneCaptures)
	{
		if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
		{
			if (USkeletalMeshComponent* CharacterMesh = Character->GetMesh())
			{
				CaptureComponent->HideComponent(CharacterMesh);
			}
		}
		CaptureComponent->TextureTarget->UpdateResourceImmediate();
		CaptureComponent->RegisterComponent();
	}

	IndirectMeshComponent->RegisterComponent();

	UE_LOG(LogTemp, Warning, TEXT("[FLXRFlux] TopRT Format: %d, Size: %dx%d"),
	       TopSceneCaptureComponent->TextureTarget->GetFormat(),
	       TopSceneCaptureComponent->TextureTarget->SizeX,
	       TopSceneCaptureComponent->TextureTarget->SizeY);

	UE_LOG(LogTemp, Warning, TEXT("[FLXRFlux] BotRT Format: %d, Size: %dx%d"),
	       BotSceneCaptureComponent->TextureTarget->GetFormat(),
	       BotSceneCaptureComponent->TextureTarget->SizeX,
	       BotSceneCaptureComponent->TextureTarget->SizeY);


	UE_LOG(LogLXRFlux, Log, TEXT("%s Created SceneCapture Component"), *GetOwner()->GetName());

	for (const auto& SceneCapture : SceneCaptures)
	{
		if (!SceneCapture || !SceneCapture->TextureTarget) continue;

		UTextureRenderTarget2D* RT = SceneCapture->TextureTarget;

		UE_LOG(LogTemp, Warning, TEXT("[FLXRFlux] SceneCapture: %s"), *SceneCapture->GetName());
		UE_LOG(LogTemp, Warning, TEXT("  ├─ Size: %dx%d"), RT->SizeX, RT->SizeY);
		UE_LOG(LogTemp, Warning, TEXT("  ├─ Format (PF): %d"), RT->GetFormat()); // PF enum
		UE_LOG(LogTemp, Warning, TEXT("  ├─ RenderTargetFormat (RTF): %d"), (int32)RT->RenderTargetFormat);
		UE_LOG(LogTemp, Warning, TEXT("  ├─ GPU Shared: %s"), RT->bGPUSharedFlag ? TEXT("Yes") : TEXT("No"));
		UE_LOG(LogTemp, Warning, TEXT("  ├─ Clear Color: R=%.3f G=%.3f B=%.3f A=%.3f"),
		       RT->ClearColor.R, RT->ClearColor.G, RT->ClearColor.B, RT->ClearColor.A);

		UE_LOG(LogTemp, Warning, TEXT("  └─ Capture Source: %s"),
		       *UEnum::GetValueAsString(SceneCapture->CaptureSource));

		FString ShowFlagsString = SceneCapture->ShowFlags.ToString();
		UE_LOG(LogTemp, Verbose, TEXT("  ├─ ShowFlags: %s"), *ShowFlagsString);
	}
}

void ULXRFluxLightDetectorComponent::UpdatePostprocessingLumenSettings()
{
	for (const auto& SceneCapture : SceneCaptures)
	{
		SceneCapture->PostProcessSettings.LumenSceneLightingQuality = bFLXRFluxLumenSceneLightingQuality;
		SceneCapture->PostProcessSettings.LumenSceneDetail = bFLXRFluxLumenSceneDetail;
		SceneCapture->PostProcessSettings.LumenSceneViewDistance = bFLXRFluxLumenSceneViewDistance;
		SceneCapture->PostProcessSettings.LumenFinalGatherQuality = bFLXRFluxLumenFinalGatherQuality;
		SceneCapture->PostProcessSettings.LumenMaxTraceDistance = bFLXRFluxLumenMaxTraceDistance;
		SceneCapture->PostProcessSettings.LumenSurfaceCacheResolution = bFLXRFluxLumenSurfaceCacheResolution;
		SceneCapture->PostProcessSettings.LumenSceneLightingUpdateSpeed = bFLXRFluxLumenSceneLightingUpdateSpeed;
		SceneCapture->PostProcessSettings.LumenFinalGatherLightingUpdateSpeed = bFLXRFluxLumenFinalGatherLightingUpdateSpeed;
		SceneCapture->PostProcessSettings.LumenDiffuseColorBoost = bFLXRFluxLumenDiffuseColorBoost;
		SceneCapture->PostProcessSettings.LumenSkylightLeaking = bFLXRFluxLumenSkylightLeaking;
		SceneCapture->PostProcessSettings.LumenFullSkylightLeakingDistance = bFLXRFluxLumenFullSkylightLeakingDistance;
	}
}


void ULXRFluxLightDetectorComponent::PrepareChildActor()
{
	TArray<UActorComponent*> ChildActorComponents;
	GetOwner()->GetComponents(UChildActorComponent::StaticClass(), ChildActorComponents, false);
	for (const auto& Element : ChildActorComponents)
	{
		if (Cast<UChildActorComponent>(Element)->GetChildActorClass()->IsChildOf(ALXRFLuxLightDetectorChildActor::StaticClass()))
		{
			ChildActorComponent = Cast<UChildActorComponent>(Element);
		}
	}
	if (!ChildActorComponent)
	{
		ChildActorComponent = Cast<UChildActorComponent>(GetOwner()->AddComponentByClass(UChildActorComponent::StaticClass(), false, FTransform::Identity, false));
		Cast<UChildActorComponent>(ChildActorComponent)->SetChildActorClass(ALXRFLuxLightDetectorChildActor::StaticClass());
		ChildActorComponent.Get()->SetupAttachment(GetOwner()->GetRootComponent());
		ChildActorComponent.Get()->RegisterComponent();
	}
}

// // Function to initiate an asynchronous texture read
// void ULXRFluxLightDetector::StartTextureReadAsync(UTextureRenderTarget2D* Texture, FTextureReadbackResult& OutResult)
// {
// 	FTextureReadbackTask* ReadbackTask = new FTextureReadbackTask(Texture, OutResult);
// 	TGraphTask<FTextureReadbackTask>::CreateTask().ConstructAndDispatchWhenReady(ReadbackTask);
// }
