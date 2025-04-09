#pragma once

#include "CoreMinimal.h"
#include "GenericPlatform/GenericPlatformMisc.h"

class ULXRFluxLightDetector;

struct FLXRFluxAnalyzeDispatchParams
{
	FRenderTarget* RenderTargetTop = nullptr;
	FRenderTarget* RenderTargetBot = nullptr;

	TUniquePtr<FRHIGPUBufferReadback> DataReadbackBuffer;

	ULXRFluxLightDetector* IndirectDetector = nullptr;

	TQueue<float> CaptureHistory;

	int32 LuminanceSum;
	int32 LuminanceCount;

	bool bAnalyzePending = false;
	bool bAnalyzeDone = false;
	bool bReadingInProgress = false;
	bool bIsInitialized = false;

	int32 PollingAttempts;


	FGPUFenceRHIRef Fence;

	TFunction<void(float Luminance,FLinearColor FinalColor)> OnReadbackComplete;

private:
	FIntPoint CachedRenderTargetSize = FIntPoint(-1, -1);
	FIntPoint CachedViewportSize = FIntPoint(-1, -1);

public:
	FLXRFluxAnalyzeDispatchParams(): LuminanceSum(0), LuminanceCount(0), PollingAttempts(0)
	{
	}

	FIntPoint GetViewportSize()
	{
		if (CachedViewportSize.X == -1)
		{
			FViewport* ViewPort = GEditor->GetActiveViewport();

			CachedViewportSize = ViewPort ? ViewPort->GetSizeXY() : FIntPoint(1920, 1080);
		}
		return CachedViewportSize;
	}

	FIntPoint GetRenderTargetSize()
	{
		if (CachedRenderTargetSize.X == -1 && RenderTargetTop)
		{
			CachedRenderTargetSize = RenderTargetTop->GetSizeXY();
		}

		return CachedRenderTargetSize;
	}

	void SetRenderTargetSize(int32 InX, int32 InY)
	{
		CachedRenderTargetSize = FIntPoint(InX, InY);
	}
};

// This is a public interface that we define so outside code can invoke our compute shader.
class LXRFLUX_API FLXRFluxCaptureInterface
{
public:
	void Dispatch(TSharedPtr<FLXRFluxAnalyzeDispatchParams> DispatchParams, FRDGBuilder& GraphBuilder, const FSceneTextures& SceneTextures)
	{
		// Prevent re-dispatch if a previous dispatch is still processing
		if (DispatchParams->bReadingInProgress || !DispatchParams->bIsInitialized || DispatchParams->bAnalyzeDone)
		{
			return;
		}

		DispatchParams->bAnalyzePending = true; // mark as queued

		if (IsInRenderingThread())
		{
			// UE_LOG(LogTemp, Log, TEXT("[FLXRFlux] DispatchRenderThread: %d"), GFrameNumberRenderThread);
			DispatchRenderThread(GraphBuilder, SceneTextures,DispatchParams);
		}
	}

	FLXRFluxCaptureInterface() = default;
	void DispatchRenderThread(FRDGBuilder& GraphBuilder, const FSceneTextures& SceneTextures, TSharedPtr<FLXRFluxAnalyzeDispatchParams> Params);
	void BeginPollingReadback(TSharedPtr<FLXRFluxAnalyzeDispatchParams> Params);

};
