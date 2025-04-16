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

#include "LXRFluxAnalyze.h"

#include "LXRBufferReadback.h"
#include "PixelShaderUtils.h"
#include "MeshPassProcessor.inl"
#include "StaticMeshResources.h"
#include "RenderGraphResources.h"
#include "GlobalShader.h"
#include "LXRFlux.h"
#include "LXRFluxLightDetector.h"
#include "RHIGPUReadback.h"
#include "MeshPassUtils.h"
#include "SceneRenderTargetParameters.h"
#include "ScreenPass.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Math/UnitConversion.h"

DECLARE_STATS_GROUP(TEXT("FLXRFluxCapture"), STATGROUP_FLXRFluxCapture, STATCAT_Advanced);

DECLARE_CYCLE_STAT(TEXT("FLXRFluxCapture Execute"), STAT_FLXRFluxCapture_Execute, STATGROUP_FLXRFluxCapture);

// This class carries our parameter declarations and acts as the bridge between cpp and HLSL.
class LXRFLUX_API FLXRFluxIndirectAnalyze : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FLXRFluxIndirectAnalyze);
	SHADER_USE_PARAMETER_STRUCT(FLXRFluxIndirectAnalyze, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters,)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D<float4>, InScene_Top)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D<float4>, InScene_Bot)

		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<uint>, OutData)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<uint>, OutCount)
		SHADER_PARAMETER(float, LuminanceThreshold)
	END_SHADER_PARAMETER_STRUCT()

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		const FPermutationDomain PermutationVector(Parameters.PermutationId);

		OutEnvironment.SetDefine(TEXT("THREADS_X"), NUM_THREADS_X);
		OutEnvironment.SetDefine(TEXT("THREADS_Y"), NUM_THREADS_Y);
		OutEnvironment.SetDefine(TEXT("THREADS_Z"), NUM_THREADS_Z);
		OutEnvironment.SetDefine(TEXT("LUMINANCE_SCALE"), LUMINANCE_SCALE);
	}

private:
};


IMPLEMENT_GLOBAL_SHADER(FLXRFluxIndirectAnalyze, "/LXRFlux_Shaders/Analyze.usf", "MainReadData", SF_Compute);

void FLXRFluxCaptureInterface::DispatchRenderThread(FRDGBuilder& GraphBuilder, const FSceneTextures& SceneTextures, TSharedPtr<FLXRFluxAnalyzeDispatchParams> DispatchParams)
{
	FRHICommandListImmediate& RHICmdList = GraphBuilder.RHICmdList;

	SCOPE_CYCLE_COUNTER(STAT_FLXRFluxCapture_Execute);
	DECLARE_GPU_STAT(FLXRFluxCapture)
	RDG_EVENT_SCOPE(GraphBuilder, "FLXRFluxCapture");
	RDG_GPU_STAT_SCOPE(GraphBuilder, FLXRFluxCapture);
	SCOPED_DRAW_EVENT(RHICmdList, STAT_FLXRFluxCapture_Execute);

	constexpr uint32 FLXRFluxIndirectBufferElements = 5;
	constexpr uint32 FLXRFluxIndirectBufferBytes = FLXRFluxIndirectBufferElements * sizeof(uint32);


	TShaderMapRef<FLXRFluxIndirectAnalyze> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	if (ComputeShader.IsValid())
	{
		DispatchParams->Fence = RHICreateGPUFence(TEXT("CaptureFence"));

		FTextureRHIRef InputRHI_Top = DispatchParams->RenderTargetTop->GetRenderTargetTexture();
		FTextureRHIRef InputRHI_Bot = DispatchParams->RenderTargetBot->GetRenderTargetTexture();

		FRDGTextureRef SceneHDR_Top = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(InputRHI_Top, TEXT("SceneHDR_Top")));
		FRDGTextureRef SceneHDR_Bot = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(InputRHI_Bot, TEXT("SceneHDR_Bot")));

		FRDGBufferRef OutDataBuffer = GraphBuilder.CreateBuffer(FRDGBufferDesc::CreateBufferDesc(sizeof(uint32), 5), TEXT("OutputDataBuffer"));

		auto* PassParams = GraphBuilder.AllocParameters<FLXRFluxIndirectAnalyze::FParameters>();
		PassParams->InScene_Top = SceneHDR_Top;
		PassParams->InScene_Bot = SceneHDR_Bot;

		PassParams->OutData = GraphBuilder.CreateUAV(FRDGBufferUAVDesc(OutDataBuffer, PF_R32_UINT));
		PassParams->LuminanceThreshold = DispatchParams->LuminanceThreshold;


		DispatchParams->DataReadbackBuffer = MakeUnique<FLXRBufferReadback>(TEXT("DataReadBack"));
		// DispatchParams->DataReadbackBuffer->EnqueueCopy(GraphBuilder, OutDataBuffer, FLXRFluxIndirectBufferBytes);


		AddClearUAVPass(GraphBuilder, PassParams->OutData, 0);


		FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("IndirectAnalyze"), ComputeShader, PassParams,
		                             FIntVector(FMath::DivideAndRoundUp(DispatchParams->RenderTargetTop->GetSizeXY().X, 8), FMath::DivideAndRoundUp(DispatchParams->RenderTargetTop->GetSizeXY().Y, 8), 1));


		DispatchParams->DataReadbackBuffer->EnqueueCopy(GraphBuilder, OutDataBuffer, FLXRFluxIndirectBufferBytes);

		GraphBuilder.AddPass(
			RDG_EVENT_NAME("IndirectAnalyze_Finalize"),
			ERDGPassFlags::NeverCull,
			[DispatchParams](FRHICommandListImmediate& RHICmdList)
			{
				if (DispatchParams->Fence.IsValid())
				{
					RHICmdList.WriteGPUFence(DispatchParams->Fence);
					DispatchParams->bAnalyzeDone = true;
					DispatchParams->bAnalyzePending = false;
				}
			});

		BeginPollingReadback(DispatchParams);
	}
}

void FLXRFluxCaptureInterface::BeginPollingReadback(TSharedPtr<FLXRFluxAnalyzeDispatchParams> DispatchParams)
{
	DispatchParams->PollingAttempts++;

	if (IsEngineExitRequested() ||
		!IsValid(DispatchParams->IndirectDetector) ||
		!DispatchParams->DataReadbackBuffer)
	{
		UE_LOG(LogTemp, Warning, TEXT("[FLXRFlux] Skipping readback polling — Game ended or object destroyed."));
		return;
	}

	if (DispatchParams->PollingAttempts > 100)
	{
		UE_LOG(LogTemp, Warning, TEXT("[FLXRFlux] Readback polling exceeded max attempts, aborting."));
		return;
	}
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 5
	const bool bReady = DispatchParams->Fence.IsValid() && DispatchParams->Fence->Poll();
#else
	const bool bReady = DispatchParams->DataReadbackBuffer && DispatchParams->DataReadbackBuffer->IsReady();
#endif

	if (bReady)
	{
		// const uint32* DataBuffer = static_cast<const uint32*>(DispatchParams->DataReadbackBuffer->Lock(5 * sizeof(uint32)));
		const uint32* DataBuffer = static_cast<const uint32*>(DispatchParams->DataReadbackBuffer->Lock(5 * sizeof(uint32)));

		constexpr float LuminanceScale = LUMINANCE_SCALE;

		uint32 EncodedR = DataBuffer[0];
		uint32 EncodedG = DataBuffer[1];
		uint32 EncodedB = DataBuffer[2];
		uint32 Count = DataBuffer[3];
		uint32 EncodedLuminance = DataBuffer[4];


		float FinalR = EncodedR / LuminanceScale;
		float FinalG = EncodedG / LuminanceScale;
		float FinalB = EncodedB / LuminanceScale;
		float Luminance = EncodedLuminance / LuminanceScale;
		// if (Count > 0)
		// {
		// 	FinalR = EncodedR / LuminanceScale / Count;
		// 	FinalG = EncodedG / LuminanceScale / Count;
		// 	FinalB = EncodedB / LuminanceScale / Count;
		// }

		DispatchParams->DataReadbackBuffer->Unlock();
		DispatchParams->DataReadbackBuffer.Reset();
		DispatchParams->bAnalyzePending = false;
		DispatchParams->bAnalyzeDone = false;
		DispatchParams->PollingAttempts = 0;

		UE_LOG(LogTemp, VeryVerbose, TEXT("[FLXRFlux] Raw Count: %u"), Count);
		UE_LOG(LogTemp, VeryVerbose, TEXT("[FLXRFlux] Max Luminance: %.4f"), Luminance);
		UE_LOG(LogTemp, VeryVerbose, TEXT("[FLXRFlux] RGB: R=%.4f G=%.4f B=%.4f"), FinalR, FinalG, FinalB);
		UE_LOG(LogTemp, VeryVerbose, TEXT("[FLXRFlux] Final Luminance Received: %f"), Luminance);

		if (DispatchParams->OnReadbackComplete)
		{
			DispatchParams->OnReadbackComplete(Luminance, FLinearColor(FinalR, FinalG, FinalB, 1));
		}
	}
	else
	{
		if (DispatchParams->PollingAttempts % 10 == 0)
		{
			UE_LOG(LogTemp, Verbose, TEXT("[FLXRFlux] Still polling readback... Attempts: %d"), DispatchParams->PollingAttempts);
		}

		TWeakPtr<FLXRFluxAnalyzeDispatchParams> WeakParams = DispatchParams;

		FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateLambda([this, WeakParams](float DeltaTime)
			{
				AsyncTask(ENamedThreads::ActualRenderingThread, [this, WeakParams]()
				{
					if (TSharedPtr<FLXRFluxAnalyzeDispatchParams> Pinned = WeakParams.Pin())
					{
						BeginPollingReadback(Pinned);
					}
					else
					{
						UE_LOG(LogTemp, Warning, TEXT("[FLXRFlux] Polling skipped — DispatchParams no longer valid."));
					}
				});
				return false;
			}),
			0.05f);
	}
}
