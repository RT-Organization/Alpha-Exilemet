#include "BaseCamp.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "AlphaExilemetCharacter.h"

// -------------------------------------------------------------------------
// CONSTRUCTOR & SETUP
// -------------------------------------------------------------------------
ABaseCamp::ABaseCamp()
{
	PrimaryActorTick.bCanEverTick = false; 

	// 1. Component Setup
	OxygenSphere = CreateDefaultSubobject<USphereComponent>(TEXT("OxygenSphere"));
	OxygenSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	RootComponent = OxygenSphere;

	// 2. Default Progression Math Setup
	ScrubberProgression.BaseValue = 500.0f; 
	ScrubberProgression.AdditivePerLevel = 1500.0f; 

	ScannerProgression.BaseValue = 0.0f; 
	ScannerProgression.AdditivePerLevel = 1000.0f; 

	DampenerProgression.BaseValue = 1.0f; 
	DampenerProgression.AdditivePerLevel = 0.0f;
	DampenerProgression.MultiplierPerLevel = 0.95f; // 5% damage reduction per level

	RetrieverProgression.BaseValue = 0.0f;
	RetrieverProgression.AdditivePerLevel = 5.0f; // +5% retained per level

	RefinerProgression.BaseValue = 1.0f;
	RefinerProgression.AdditivePerLevel = 0.05f; // +5% payout per level

	// Initialize Sphere size
	OxygenRegenRadius = ScrubberProgression.BaseValue;
	OxygenSphere->InitSphereRadius(OxygenRegenRadius);

	// 3. Bind Events
	OxygenSphere->OnComponentBeginOverlap.AddDynamic(this, &ABaseCamp::OnOverlapBegin);
	OxygenSphere->OnComponentEndOverlap.AddDynamic(this, &ABaseCamp::OnOverlapEnd);
}

// -------------------------------------------------------------------------
// ENGINE OVERRIDES
// -------------------------------------------------------------------------
void ABaseCamp::BeginPlay()
{
	Super::BeginPlay();
	
	// Apply saved upgrades to the world immediately
	ApplyShipUpgrades(); 
	
	// Check if the player spawned inside the bubble
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

void ABaseCamp::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	
	if (OxygenSphere)
	{
		OxygenSphere->SetSphereRadius(OxygenRegenRadius);
	}

	BP_UpdateForcefieldRadius(OxygenRegenRadius);
}

// -------------------------------------------------------------------------
// SHIP REPAIR PROGRESSION
// -------------------------------------------------------------------------
int32 ABaseCamp::GetShipSystemLevel(EShipSystem SystemID)
{
	if (ShipRepairLevels.Contains(SystemID))
	{
		return ShipRepairLevels[SystemID];
	}
	return 0; 
}

void ABaseCamp::UpgradeShipSystem(EShipSystem SystemID)
{
	if (ShipRepairLevels.Contains(SystemID))
	{
		ShipRepairLevels[SystemID]++; 
	}
	else
	{
		ShipRepairLevels.Add(SystemID, 1); 
	}

	ApplyShipUpgrades(); 
}

void ABaseCamp::ApplyShipUpgrades()
{
	// 1. Atmospheric Scrubber (Oxygen Bubble)
	int32 ScrubberLevel = GetShipSystemLevel(EShipSystem::AtmosphericScrubber);
	OxygenRegenRadius = ScrubberProgression.GetValueAtLevel(ScrubberLevel);
	
	if (OxygenSphere)
	{
		OxygenSphere->SetSphereRadius(OxygenRegenRadius);
	}
	BP_UpdateForcefieldRadius(OxygenRegenRadius);

	// 2. Topography Scanner (Player Outline Sphere)
	int32 ScannerLevel = GetShipSystemLevel(EShipSystem::TopographyScanner);
	float NewScannerRadius = ScannerProgression.GetValueAtLevel(ScannerLevel);

	if (AAlphaExilemetCharacter* Player = Cast<AAlphaExilemetCharacter>(GetWorld()->GetFirstPlayerController()->GetPawn()))
	{
		if (Player->ScannerSphere)
		{
			Player->ScannerSphere->SetSphereRadius(NewScannerRadius);
		}
	}

	// Note: Systems 3 (Dampener), 4 (Retriever), and 5 (Refiner) don't need physical 
	// world updates here. The player/widgets will just call GetShipSystemLevel() 
	// when calculating damage, death, or selling!
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
	}
}

void ABaseCamp::UnlockSpecialItem(ESpecialItem ItemID)
{
	if (!UnlockedSpecialItems.Contains(ItemID))
	{
		UnlockedSpecialItems.Add(ItemID);
	}
}

// -------------------------------------------------------------------------
// EVENT HANDLERS
// -------------------------------------------------------------------------
void ABaseCamp::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (AAlphaExilemetCharacter* Character = Cast<AAlphaExilemetCharacter>(OtherActor))
	{
		if (OtherComp == Character->GetCapsuleComponent())
		{
			Character->bIsInSafeZone = true;
		}
	}
}

void ABaseCamp::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (AAlphaExilemetCharacter* Character = Cast<AAlphaExilemetCharacter>(OtherActor))
	{
		if (OtherComp == Character->GetCapsuleComponent())
		{
			Character->bIsInSafeZone = false;
		}
	}
}