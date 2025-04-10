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
