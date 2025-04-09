// Copyright Clusterfact Games 2023.  All Rights Reserved.


#include "LXRFLuxLightDetectorChildActor.h"


// Sets default values
ALXRFLuxLightDetectorChildActor::ALXRFLuxLightDetectorChildActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	// SetTickGroup(TG_PrePhysics);
	PrimaryActorTick.bCanEverTick = true;
	SetMobility(EComponentMobility::Movable);
}

// Called when the game starts or when spawned
void ALXRFLuxLightDetectorChildActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ALXRFLuxLightDetectorChildActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	 // SetActorRotation(FQuat::MakeFromEuler(FVector::RightVector),ETeleportType::TeleportPhysics);
	
	// float f = UKismetMathLibrary::Sin(GetWorld()->GetTimeSeconds());
	// SetActorRotation(FQuat::MakeFromEuler(UKismetMathLibrary::RotateAngleAxis(FVector::RightVector,90*f,FVector::UpVector)));

	// const FVector Random = UKismetMathLibrary::RandomUnitVector();
	// SetActorRelativeLocation(Random);
	// SetActorRelativeLocation(FVector::ZeroVector);
	
	// AActor* parent =  GetAttachParentActor();
	// UChildActorComponent* ChildActor = parent->FindComponentByClass<UChildActorComponent>();
	// ChildActor->SetWorldRotation(FQuat::Identity);
}

