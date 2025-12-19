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
#include "Math/UnrealMathUtility.h"
#include "Async/Async.h"
#include "Math/UnitConversion.h"


namespace LXRFluxCaptureIndex
{
	constexpr uint32 INDEX_R = 0;
	constexpr uint32 INDEX_G = 1;
	constexpr uint32 INDEX_B = 2;
	constexpr uint32 INDEX_MAX_LUM = 3;
	constexpr uint32 INDEX_COUNT = 4;
}

namespace LXRFluxCaptureConstants
{
	constexpr uint32 NUM_THREADS_X = 32;
	constexpr uint32 NUM_THREADS_Y = 32;
	constexpr uint32 NUM_THREADS_Z = 1;
	constexpr float LUMINANCE_SCALE = 10000.0f;

	constexpr uint32 FLXRFluxBufferElements = 5;
	constexpr uint32 FLXRFluxBufferBytes = FLXRFluxBufferElements * sizeof(uint32);
}


DECLARE_STATS_GROUP(TEXT("FLXRFluxCapture"), STATGROUP_FLXRFluxCapture, STATCAT_Advanced);

DECLARE_CYCLE_STAT(TEXT("FLXRFluxCapture Execute"), STAT_FLXRFluxCapture_Execute, STATGROUP_FLXRFluxCapture);

// This class carries our parameter declarations and acts as the bridge between cpp and HLSL.
class LXRFLUX_API FLXRFluxIndirectAnalyze : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FLXRFluxIndirectAnalyze);
	SHADER_USE_PARAMETER_STRUCT(FLXRFluxIndirectAnalyze, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters,)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D<float4>, InSceneCapture)

		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<uint>, OutData)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<uint>, OutCount)
		SHADER_PARAMETER(float, LuminanceThreshold)
	END_SHADER_PARAMETER_STRUCT()

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		const FPermutationDomain PermutationVector(Parameters.PermutationId);

		OutEnvironment.SetDefine(TEXT("THREADS_X"), LXRFluxCaptureConstants::NUM_THREADS_X);
		OutEnvironment.SetDefine(TEXT("THREADS_Y"), LXRFluxCaptureConstants::NUM_THREADS_Y);
		OutEnvironment.SetDefine(TEXT("THREADS_Z"), LXRFluxCaptureConstants::NUM_THREADS_Z);
		OutEnvironment.SetDefine(TEXT("LUMINANCE_SCALE"), LXRFluxCaptureConstants::LUMINANCE_SCALE);
	}

private:
};


IMPLEMENT_GLOBAL_SHADER(FLXRFluxIndirectAnalyze, "/LXRFlux_Shaders/Analyze.usf", "MainReadData", SF_Compute);

void FLXRFluxCaptureInterface::DispatchRenderThread(FRDGBuilder& GraphBuilder, const FSceneTextures& SceneTextures, TSharedPtr<FLXRFluxAnalyzeDispatchParams> DispatchParams)
{
	FRHICommandListImmediate& RHICmdList = GraphBuilder.RHICmdList;
	
	SCOPE_CYCLE_COUNTER(STAT_FLXRFluxCapture_Execute);
	DECLARE_GPU_STAT(FLXRFluxCapture);
	RDG_EVENT_SCOPE(GraphBuilder, "FLXRFluxCapture");
	RDG_GPU_STAT_SCOPE(GraphBuilder, FLXRFluxCapture);
	SCOPED_DRAW_EVENT(RHICmdList, STAT_FLXRFluxCapture_Execute);


	TShaderMapRef<FLXRFluxIndirectAnalyze> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	if (ComputeShader.IsValid())
	{
		DispatchParams->Fence = RHICreateGPUFence(TEXT("CaptureFence"));

		ELXRFluxCaptureStep CurrentStep = static_cast<ELXRFluxCaptureStep>(DispatchParams->CaptureStepCounter);
		FTextureRHIRef InputRHI = CurrentStep == ELXRFluxCaptureStep::Top
			                          ? DispatchParams->RenderTargetTop->GetRenderTargetTexture()
			                          : DispatchParams->RenderTargetBot->GetRenderTargetTexture();


		if (!InputRHI.IsValid()) return;

		FRDGTextureRef SceneHDR = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(InputRHI, TEXT("SceneHDR")));
		// FRDGTextureRef SceneHDR_Bot = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(InputRHI_Bot, TEXT("SceneHDR_Bot")));

		FRDGBufferRef OutDataBuffer = GraphBuilder.CreateBuffer(FRDGBufferDesc::CreateBufferDesc(sizeof(uint32), LXRFluxCaptureConstants::FLXRFluxBufferElements), TEXT("OutputDataBuffer"));

		auto* PassParams = GraphBuilder.AllocParameters<FLXRFluxIndirectAnalyze::FParameters>();
		PassParams->InSceneCapture = SceneHDR;
		// PassParams->InScene_Bot = SceneHDR_Bot;

		PassParams->OutData = GraphBuilder.CreateUAV(FRDGBufferUAVDesc(OutDataBuffer, PF_R32_UINT));
		PassParams->LuminanceThreshold = DispatchParams->LuminanceThreshold;


		DispatchParams->DataReadbackBuffer = MakeUnique<FLXRBufferReadback>(TEXT("DataReadBack"));
		// DispatchParams->DataReadbackBuffer->EnqueueCopy(GraphBuilder, OutDataBuffer, FLXRFluxIndirectBufferBytes);


		AddClearUAVPass(GraphBuilder, PassParams->OutData, 0);

		int NumThreadsX = LXRFluxCaptureConstants::NUM_THREADS_X;
		int NumThreadsY = LXRFluxCaptureConstants::NUM_THREADS_Y;
		const FIntPoint Size = (CurrentStep == ELXRFluxCaptureStep::Top)
			                       ? DispatchParams->RenderTargetTop->GetSizeXY()
			                       : DispatchParams->RenderTargetBot->GetSizeXY();

		FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("IndirectAnalyze"), ComputeShader, PassParams,
		                             FIntVector(
			                             FMath::DivideAndRoundUp(Size.X, NumThreadsX),
			                             FMath::DivideAndRoundUp(Size.Y, NumThreadsY),
			                             1));


		DispatchParams->DataReadbackBuffer->EnqueueCopy(GraphBuilder, OutDataBuffer, LXRFluxCaptureConstants::FLXRFluxBufferBytes);
		DispatchParams->bReadbackInFlight.Store(true);


		GraphBuilder.AddPass(
			RDG_EVENT_NAME("IndirectAnalyze_Finalize"),
			ERDGPassFlags::NeverCull,
			[DispatchParams](FRHICommandListImmediate& RHICmdList)
			{
				if (DispatchParams->Fence.IsValid())
				{
					RHICmdList.WriteGPUFence(DispatchParams->Fence);
				}
			});
	}
}


void FLXRFluxCaptureInterface::BeginPollingReadback_RenderThread(TSharedPtr<FLXRFluxAnalyzeDispatchParams> Params)
{
	check(IsInRenderingThread());
	if (!Params.IsValid()) return;
	if (IsEngineExitRequested()) return;
	if (!Params->bReadbackInFlight.Load()) return;

	if (!Params->DataReadbackBuffer) return;

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 5
	const bool bReady = Params->Fence.IsValid() && Params->Fence->Poll();
#else
	const bool bReady = Params->DataReadbackBuffer->IsReady();
#endif

	if (!bReady)
		return;

	const uint32* DataBuffer = static_cast<const uint32*>(Params->DataReadbackBuffer->Lock(LXRFluxCaptureConstants::FLXRFluxBufferBytes));

	constexpr float LuminanceScale = LXRFluxCaptureConstants::LUMINANCE_SCALE;

	uint32 EncodedR = DataBuffer[LXRFluxCaptureIndex::INDEX_R];
	uint32 EncodedG = DataBuffer[LXRFluxCaptureIndex::INDEX_G];
	uint32 EncodedB = DataBuffer[LXRFluxCaptureIndex::INDEX_B];
	uint32 EncodedLuminance = DataBuffer[LXRFluxCaptureIndex::INDEX_MAX_LUM];
	uint32 Count = DataBuffer[LXRFluxCaptureIndex::INDEX_COUNT];


	float FinalR = EncodedR / LuminanceScale;
	float FinalG = EncodedG / LuminanceScale;
	float FinalB = EncodedB / LuminanceScale;
	float Luminance = EncodedLuminance / LuminanceScale;

	FLinearColor FinalColor = FLinearColor(FinalR, FinalG, FinalB, 1);


	ELXRFluxCaptureStep CurrentStep = static_cast<ELXRFluxCaptureStep>(Params->CaptureStepCounter);

	if (CurrentStep == ELXRFluxCaptureStep::Top)
	{
		Params->Output->TopColor = FinalColor;
		Params->Output->TopLuminance = Luminance;
	}
	else
	{
		Params->Output->BotColor = FinalColor;
		Params->Output->BotLuminance = Luminance;
	}

	Params->DataReadbackBuffer->Unlock();
	Params->DataReadbackBuffer.Reset();
	Params->bReadbackInFlight.Store(false);

	AsyncTask(ENamedThreads::GameThread, [Params]()
	{
		Params->bAnalyzePending = false;
		Params->bAnalyzeDone = true;

		if (Params->OnReadbackComplete)
			Params->OnReadbackComplete();
	});
}

