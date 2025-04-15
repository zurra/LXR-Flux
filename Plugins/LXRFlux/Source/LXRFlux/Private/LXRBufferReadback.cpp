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

#include "LXRBufferReadback.h"
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