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


#include "LXRFluxCaptureSubSystem.h"
#include "LXRFluxAnalyze.h"
#include "LXRFluxLightDetector.h"
#include "Modules/ModuleManager.h"
#include "Engine/TextureRenderTarget2D.h"

ULXRFluxSubSystem::ULXRFluxSubSystem(const FObjectInitializer& ObjectInitializer)
{
	ConstructorHelpers::FObjectFinder<UMaterialInterface> IndirectPostProcessFinder(TEXT("/LXR/Materials/PP_LXR_HDR_FULLRANGE.PP_LXR_HDR_FULLRANGE"));
	if (IndirectPostProcessFinder.Succeeded())
	{
		IndirectPostProcessMaterial = IndirectPostProcessFinder.Object;
	}

	CaptureInterface = MakeUnique<FLXRFluxCaptureInterface>();
}

FString ULXRFluxSubSystem::GetLXRAssetPath()
{
	FString Path;
	if (FModuleManager::Get().IsModuleLoaded(TEXT("LXR")))
	{
		Path = "/LXR";
	}
	else
	{
		Path = "/LXRFlux";
	}
	return Path;
}

void ULXRFluxSubSystem::Initialize(FSubsystemCollectionBase& Collection)
{
	DoCaptures();
	Super::Initialize(Collection);
}

void ULXRFluxSubSystem::Deinitialize()
{
	bStopRender = true;
	PendingAnalyzeQueue.Empty();
	Super::Deinitialize();
}

void ULXRFluxSubSystem::DoCaptures()
{
	const FName RendererModuleName("Renderer");
	IRendererModule* RendererModule = FModuleManager::GetModulePtr<IRendererModule>(RendererModuleName);
	if (RendererModule)
	{
		if (OnPostResolvedSceneColorHandle.IsValid())
		{
			RendererModule->GetResolvedSceneColorCallbacks().Remove(OnPostResolvedSceneColorHandle);
		}

		OnPostResolvedSceneColorHandle =
			RendererModule->
			GetResolvedSceneColorCallbacks().
			AddWeakLambda(this,
			              [this](FRDGBuilder& GraphBuilder, const FSceneTextures& SceneTextures)
			              {
				              TSharedPtr<FLXRFluxAnalyzeDispatchParams> Params;
				              if (bStopRender) return;
				              while (PendingAnalyzeQueue.Dequeue(Params))
				              {
					              if (!Params.IsValid() || !Params->IndirectDetector) continue;

					              if (Params->RenderTargetTop && Params->RenderTargetBot)
					              {
						              CaptureInterface->Dispatch(Params, GraphBuilder, SceneTextures);
					              }
				              }
			              });
	}
}

void ULXRFluxSubSystem::RequestIndirectAnalyze(TSharedPtr<FLXRFluxAnalyzeDispatchParams> Params)
{
	if (Params.IsValid() && !Params->bAnalyzePending)
	{
		Params->bAnalyzePending = true;
		PendingAnalyzeQueue.Enqueue(Params);
	}
}

void ULXRFluxSubSystem::StartABTestIndividualGISettings(UWorld* World)
{
	if (!World) return;

	struct FCVarTest
	{
		FString Name;
		float LowValue;
		float HighValue;
		float DefaultValue;
	};


	TArray<FCVarTest> CVars = {
		{TEXT("r.DistanceFieldAO"), 1, 1},
		{TEXT("r.AOQuality"), 1, 1},
		{TEXT("r.Lumen.DiffuseIndirect.Allow"), 0, 1},
		{TEXT("r.SkyLight.RealTimeReflectionCapture"), 0, 1},
		{TEXT("r.RayTracing.Scene.BuildMode"), 0, 0},
		{TEXT("r.LumenScene.Radiosity.ProbeSpacing"), 64, 8},
		{TEXT("r.LumenScene.Radiosity.HemisphereProbeResolution"), 1, 3},
		{TEXT("r.Lumen.TraceMeshSDFs.Allow"), 0, 0},
		{TEXT("r.Lumen.ScreenProbeGather.RadianceCache.ProbeResolution"), 4, 16},
		{TEXT("r.Lumen.ScreenProbeGather.RadianceCache.NumProbesToTraceBudget"), 50, 300},
		{TEXT("r.Lumen.ScreenProbeGather.DownsampleFactor"), 64, 32},
		{TEXT("r.Lumen.ScreenProbeGather.TracingOctahedronResolution"), 4, 8},
		{TEXT("r.Lumen.ScreenProbeGather.IrradianceFormat"), 0, 1},
		{TEXT("r.Lumen.ScreenProbeGather.StochasticInterpolation"), 0, 1},
		{TEXT("r.Lumen.ScreenProbeGather.FullResolutionJitterWidth"), 0.5, 1},
		{TEXT("r.Lumen.ScreenProbeGather.TwoSidedFoliageBackfaceDiffuse"), 0, 0},
		{TEXT("r.Lumen.ScreenProbeGather.ScreenTraces.HZBTraversal.FullResDepth"), 0, 0},
		{TEXT("r.Lumen.ScreenProbeGather.ShortRangeAO.HardwareRayTracing"), 0, 0},
		{TEXT("r.Lumen.TranslucencyVolume.GridPixelSize"), 128, 64},
		{TEXT("r.Lumen.TranslucencyVolume.TraceFromVolume"), 0, 0},
		{TEXT("r.Lumen.TranslucencyVolume.RadianceCache.ProbeResolution"), 4, 8},
		{TEXT("r.Lumen.TranslucencyVolume.RadianceCache.NumProbesToTraceBudget"), 100, 200},
	};

	IConsoleManager& ConsoleMgr = IConsoleManager::Get();
	for (FCVarTest& CVar : CVars)
	{
		IConsoleVariable* Var = ConsoleMgr.FindConsoleVariable(*CVar.Name);
		Var->GetValue(CVar.DefaultValue);
	}

	struct FTestStep
	{
		FString CVarName;
		float Value;
		FString Label;
	};

	struct FStepState
	{
		int32 StepIndex = 0;
	};

	TArray<FTestStep> TestSteps;

	// Generate steps
	for (const FCVarTest& CVar : CVars)
	{
		IConsoleVariable* Var = ConsoleMgr.FindConsoleVariable(*CVar.Name);
		if (!Var) continue;

		TestSteps.Add({CVar.Name, CVar.LowValue, CVar.Name + TEXT("_Low")});
		TestSteps.Add({CVar.Name, CVar.HighValue, CVar.Name + TEXT("_High")});
		TestSteps.Add({CVar.Name, CVar.DefaultValue, CVar.Name + TEXT("_Default")});
	}

	// Run steps with 1-second delay between each
	FTimerHandle StepTimer;
	int32 StepIndex = 0;

	TSharedPtr<FTimerDelegate> StepDelegate = MakeShared<FTimerDelegate>();
	TSharedRef<FStepState> SharedState = MakeShared<FStepState>();


	*StepDelegate = FTimerDelegate::CreateLambda([World, SharedState, TestSteps, StepDelegate]()
	{
		if (SharedState->StepIndex >= TestSteps.Num()) return;

		const auto& Step = TestSteps[SharedState->StepIndex];
		IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(*Step.CVarName);
		float NextCvarTime = Step.Label.Contains("_Default") ? 0.25f : 1.0f;

		if (Var)
		{
			Var->Set(Step.Value);
			UE_LOG(LogTemp, Warning, TEXT("[LXR ABTest_%s] %s = %f"), *Step.Label, *Step.CVarName, Step.Value);

			GEngine->AddOnScreenDebugMessage(-1, NextCvarTime, FColor::Green, FString::Printf(TEXT("[LXR ABTest_%s] %s = %f"), *Step.Label, *Step.CVarName, Step.Value));
		}

		SharedState->StepIndex++;
		if (SharedState->StepIndex < TestSteps.Num())
		{
			FTimerHandle StepTimer;
			World->GetTimerManager().SetTimer(StepTimer, *StepDelegate, NextCvarTime, false);
		}
	});


	// Start first step
	World->GetTimerManager().SetTimer(StepTimer, *StepDelegate, 1.0f, false);
}
