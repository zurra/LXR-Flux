// Fill out your copyright notice in the Description page of Project Settings.


#include "LXRFluxCaptureSubSystem.h"
#include "LXRFluxAnalyze.h"
#include "LXRFluxLightDetector.h"
#include "Engine/TextureRenderTarget2D.h"

ULXRFluxSubSystem::ULXRFluxSubSystem(const FObjectInitializer& ObjectInitializer)
{
	ConstructorHelpers::FObjectFinder<UMaterialInterface> IndirectPostProcessFinder(TEXT("/LXRFlux/Materials/PP_FLXRFlux_HDR_FULLRANGE.PP_FLXRFlux_HDR_FULLRANGE"));
	if (IndirectPostProcessFinder.Succeeded())
	{
		IndirectPostProcessMaterial = IndirectPostProcessFinder.Object;
	}

	CaptureInterface = MakeUnique<FLXRFluxCaptureInterface>();
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
