#include "FLXRBufferReadback.h"
#include "RenderGraphUtils.h"
#include "RHICommandList.h"
#include "RHIGPUReadback.h"
#include "RenderGraph.h"

FLXRBufferReadback::FLXRBufferReadback(const TCHAR* InDebugName)
	: DebugName(InDebugName)
{}

void FLXRBufferReadback::EnqueueCopy(FRDGBuilder& GraphBuilder, FRDGBufferRef SrcBuffer, uint32 SizeInBytes)
{
	Size = SizeInBytes;

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 5
	TRefCountPtr<FRDGPooledBuffer> PooledBuffer = GraphBuilder.ConvertToExternalBuffer(SrcBuffer);
	FRHIBuffer* SrcRHI = PooledBuffer->GetRHI();
	GraphBuilder.AddPass(
		RDG_EVENT_NAME("FLXRBufferReadback::CopyToStaging (%s)", DebugName),
		ERDGPassFlags::NeverCull,
		[this, SrcRHI](FRHICommandListImmediate& RHICmdList)
		{
			StagingBuffer = RHICreateStagingBuffer();
			RHICmdList.CopyToStagingBuffer(SrcRHI,StagingBuffer,0,Size );

		});
#else
	if (!ReadbackBuffer.IsValid())
	{
		ReadbackBuffer = MakeUnique<FRHIGPUBufferReadback>(DebugName);
	}
	AddEnqueueCopyPass(GraphBuilder, ReadbackBuffer.Get(), SrcBuffer, SizeInBytes);
#endif
}

bool FLXRBufferReadback::IsReady() const
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 5
	return true;
#else
	return ReadbackBuffer.IsValid() && ReadbackBuffer->IsReady();
#endif
}

void* FLXRBufferReadback::Lock(uint32 NumBytes)
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 5
	return RHILockStagingBuffer(StagingBuffer, 0, NumBytes);
#else
	return ReadbackBuffer->Lock(NumBytes);
#endif
}

void FLXRBufferReadback::Unlock()
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 5
	RHIUnlockStagingBuffer(StagingBuffer);
#else
	ReadbackBuffer->Unlock();
#endif
}

void FLXRBufferReadback::Reset()
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 5
	StagingBuffer.SafeRelease();
#else
	ReadbackBuffer.Reset();
#endif
	Size = 0;
}