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
#include "Engine/TextureRenderTarget2D.h"

ULXRFluxSubSystem::ULXRFluxSubSystem(const FObjectInitializer& ObjectInitializer)
{
	ConstructorHelpers::FObjectFinder<UMaterialInterface> IndirectPostProcessFinder(TEXT("/LXRFlux/Materials/PP_LXR_HDR_FULLRANGE.PP_LXR_HDR_FULLRANGE"));
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
