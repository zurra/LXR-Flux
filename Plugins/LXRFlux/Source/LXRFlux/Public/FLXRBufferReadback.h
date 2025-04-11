#pragma once

#include "CoreMinimal.h"
#include "RenderGraphUtils.h"
#include "RHIGPUReadback.h"

class FRDGBuilder;


class FLXRBufferReadback
{
public:
	explicit FLXRBufferReadback(const TCHAR* InDebugName);

	void EnqueueCopy(FRDGBuilder& GraphBuilder, FRDGBufferRef SrcBuffer, uint32 SizeInBytes);
	bool IsReady() const;
	void* Lock(uint32 NumBytes);
	void Unlock();
	void Reset();

private:
	const TCHAR* DebugName;
	uint32 Size = 0;

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 5
	TRefCountPtr<FRHIStagingBuffer> StagingBuffer;
#else
	TUniquePtr<FRHIGPUBufferReadback> ReadbackBuffer;
#endif
};