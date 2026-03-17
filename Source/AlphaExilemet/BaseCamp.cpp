#include "BaseCamp.h"
#include "Components/SphereComponent.h"
#include "AlphaExilemetCharacter.h"

ABaseCamp::ABaseCamp()
{
	// Disable Tick completely to improve performance
	PrimaryActorTick.bCanEverTick = false; 

	OxygenRegenRadius = 500.f;
	
	OxygenSphere = CreateDefaultSubobject<USphereComponent>(TEXT("OxygenSphere"));
	OxygenSphere->InitSphereRadius(OxygenRegenRadius);
	OxygenSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	RootComponent = OxygenSphere;

	// Bind the Overlap Events
	OxygenSphere->OnComponentBeginOverlap.AddDynamic(this, &ABaseCamp::OnOverlapBegin);
	OxygenSphere->OnComponentEndOverlap.AddDynamic(this, &ABaseCamp::OnOverlapEnd);
}

void ABaseCamp::BeginPlay()
{
	Super::BeginPlay();
	OxygenSphere->SetSphereRadius(OxygenRegenRadius);
	
	TArray<AActor*> OverlappingActors;
	OxygenSphere->GetOverlappingActors(OverlappingActors, AAlphaExilemetCharacter::StaticClass());

	for (AActor* Actor : OverlappingActors)
	{
		if (AAlphaExilemetCharacter* Character = Cast<AAlphaExilemetCharacter>(Actor))
		{
			Character->bIsInSafeZone = true;
		}
	}
}

void ABaseCamp::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// If the player steps into the base camp, set them to safe
	if (AAlphaExilemetCharacter* Character = Cast<AAlphaExilemetCharacter>(OtherActor))
	{
		Character->bIsInSafeZone = true;
	}
}

void ABaseCamp::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	// If the player leaves the base camp, they are no longer safe
	if (AAlphaExilemetCharacter* Character = Cast<AAlphaExilemetCharacter>(OtherActor))
	{
		Character->bIsInSafeZone = false;
	}
}

void ABaseCamp::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	
	if (OxygenSphere)
	{
		OxygenSphere->SetSphereRadius(OxygenRegenRadius);
	}
}

// -------------------------------------------------------------------------
// SHIP REPAIR PROGRESSION
// -------------------------------------------------------------------------

int32 ABaseCamp::GetShipSystemLevel(EShipSystem SystemID)
{
	// Check if we have data for this system yet
	if (ShipRepairLevels.Contains(SystemID))
	{
		return ShipRepairLevels[SystemID];
	}
	
	// If it's not in the map, it hasn't been upgraded, so it's Level 0
	return 0; 
}

void ABaseCamp::UpgradeShipSystem(EShipSystem SystemID)
{
	if (ShipRepairLevels.Contains(SystemID))
	{
		ShipRepairLevels[SystemID]++; // Increment existing level
	}
	else
	{
		ShipRepairLevels.Add(SystemID, 1); // First upgrade, set to Level 1
	}

	// TODO Later: You can add an Event Dispatcher here like OnShipUpgraded.Broadcast(SystemID);
	// if you want the base visuals to change when an upgrade happens!
}

// -------------------------------------------------------------------------
// SHOP PROGRESSION / UNLOCKS
// -------------------------------------------------------------------------

bool ABaseCamp::IsToolUnlocked(EToolType ToolID)
{
	return UnlockedTools.Contains(ToolID);
}

bool ABaseCamp::IsSpecialItemUnlocked(ESpecialItem ItemID)
{
	return UnlockedSpecialItems.Contains(ItemID);
}

void ABaseCamp::UnlockTool(EToolType ToolID)
{
	if (!UnlockedTools.Contains(ToolID))
	{
		UnlockedTools.Add(ToolID);
		// TODO: Call a function on the Player Character to give them the actual tool!
	}
}

void ABaseCamp::UnlockSpecialItem(ESpecialItem ItemID)
{
	if (!UnlockedSpecialItems.Contains(ItemID))
	{
		UnlockedSpecialItems.Add(ItemID);
	}
}