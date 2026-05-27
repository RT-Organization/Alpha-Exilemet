// Fill out your copyright notice in the Description page of Project Settings.

#include "SolidResource.h"

#include "AlphaExilemet/Data/AlphaExilemetTypes.h"
#include "DroppedSolidResource.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

ASolidResource::ASolidResource()
{
	OreMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OreMesh"));
	OreMesh->SetupAttachment(RootComponent);
	OreMesh->SetCollisionProfileName(TEXT("BlockAll"));
	
	// Optional default sound assets.
	// Replace this path with your own asset path if different.
	static ConstructorHelpers::FObjectFinder<USoundBase> Metal1(
		TEXT("/Game/AlphaExilemet/SFX/XFX/Base/MetalImpacts/MetalImpact1.MetalImpact1")
	);
	
	static ConstructorHelpers::FObjectFinder<USoundBase> Metal2(
		TEXT("/Game/AlphaExilemet/SFX/XFX/Base/MetalImpacts/MetalImpact2.MetalImpact2")
	);
	
	static ConstructorHelpers::FObjectFinder<USoundBase> Metal3(
		TEXT("/Game/AlphaExilemet/SFX/XFX/Base/MetalImpacts/MetalImpact3.MetalImpact3")
	);
	
	if (Metal1.Succeeded())
	{
		MiningHitSounds.Add(Metal1.Object);
	}
	
	if (Metal2.Succeeded())
	{
		MiningHitSounds.Add(Metal2.Object);
	}
	
	if (Metal3.Succeeded())
	{
		MiningHitSounds.Add(Metal3.Object);
	}
}

void ASolidResource::SetLastPickaxe(APickaxeTool* Tool)
{
	LastPickaxe = Tool;
}

bool ASolidResource::ApplyResourceDamage(float DamageAmount)
{
	// Ignore hits if already depleted.
	if (bIsDepleted)
	{
		return false;
	}
	
	// Play sound before applying damage, so the final hit still produces audio
	// even if the actor is hidden during depletion.
	PlayMiningHitSound();
	
	// Let the base class handle health reduction, scaling, depletion, and regen.
	return Super::ApplyResourceDamage(DamageAmount);
}

void ASolidResource::PlayMiningHitSound() const
{
	if (MiningHitSounds.Num() == 0)
	{
		return;
	}

	int32 Index = FMath::RandRange(0, MiningHitSounds.Num() - 1);
	USoundBase* SelectedSound = MiningHitSounds[Index];

	if (!SelectedSound)
	{
		return;
	}

	const float PitchMultiplier = FMath::FRandRange(
		MiningHitPitchRange.X,
		MiningHitPitchRange.Y
	);

	const float VolumeMultiplier = FMath::FRandRange(
		MiningHitVolumeRange.X,
		MiningHitVolumeRange.Y
	);

	UGameplayStatics::PlaySoundAtLocation(
		this,
		SelectedSound,
		GetActorLocation(),
		VolumeMultiplier,
		PitchMultiplier
	);
}

void ASolidResource::UpdateScale()
{
	float HealthRatio = Health / InitialHealth;
	HealthRatio = FMath::Clamp(HealthRatio, 0.0f, 1.0f);
	
	const float ScaledValue = FMath::Lerp(0.5f, 1.0f, HealthRatio);
	
	SetActorScale3D(ScaledValue * InitialScale);
}

void ASolidResource::DepleteResource()
{
	Super::DepleteResource();
	
	const FResourceRow* Row = ResourceID.GetRow<FResourceRow>(TEXT("SolidResource"));
	if (!Row)
	{
		return;
	}
	
	const int32 BaseMaxDrops = Row->MaxDrops;
	
	int32 LuckBonus = 0;
	if (LastPickaxe)
	{
		LuckBonus = FMath::FloorToInt(LastPickaxe->GetMiningLuck());
	}
	
	LuckBonus = FMath::Clamp(LuckBonus, 0, Row->MaxLuckBoost);
	
	const int32 MaxDropsWithLuck = BaseMaxDrops + LuckBonus;
	const int32 DropCount = FMath::RandRange(1, MaxDropsWithLuck);
	
	for (int32 i = 0; i < DropCount; i++)
	{
		SpawnDroppedResource();
	}
	
	SetLastPickaxe(nullptr);
}

void ASolidResource::SpawnDroppedResource()
{
	UStaticMesh* MeshAsset = OreMesh->GetStaticMesh();
	if (!MeshAsset)
	{
		return;
	}
	
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	
	ADroppedSolidResource* Dropped = GetWorld()->SpawnActor<ADroppedSolidResource>(
		ADroppedSolidResource::StaticClass(),
		GetActorLocation(),
		FRotator::ZeroRotator,
		Params
	);
	
	if (!Dropped)
	{
		return;
	}
	
	Dropped->InitDroppedResource(MeshAsset, ResourceID);
	Dropped->SetActorScale3D(GetActorScale3D());
	
	UStaticMeshComponent* MeshComp =
		Dropped->FindComponentByClass<UStaticMeshComponent>();
	
	if (MeshComp && MeshComp->IsSimulatingPhysics())
	{
		FVector Direction;
		
		if (DropImpulseAngle >= 360.f)
		{
			Direction = FMath::VRand();
		}
		else if (DropImpulseAngle <= 0.f)
		{
			Direction = FVector::UpVector;
		}
		else
		{
			const float ConeHalfAngleRad =
				FMath::DegreesToRadians(DropImpulseAngle);
			
			Direction = FMath::VRandCone(
				FVector::UpVector,
				ConeHalfAngleRad
			);
		}
		
		const FVector Impulse = Direction * DropImpulseStrength;
		MeshComp->AddImpulse(Impulse, NAME_None, true);
	}
}