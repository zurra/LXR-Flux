// Copyright Clusterfact Games 2023.  All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/Actor.h"
#include "LXRFLuxLightDetectorChildActor.generated.h"

UCLASS(Blueprintable)
class LXRFLUX_API ALXRFLuxLightDetectorChildActor : public AStaticMeshActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ALXRFLuxLightDetectorChildActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
