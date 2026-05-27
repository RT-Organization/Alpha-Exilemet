#include "BaseCamp.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "AlphaExilemetCharacter.h"

// ─────────────────────────────────────────────────────────────────────────────
// CONSTRUCTOR & SETUP — unchanged
// ─────────────────────────────────────────────────────────────────────────────

ABaseCamp::ABaseCamp()
{
	PrimaryActorTick.bCanEverTick = false; 

	OxygenSphere = CreateDefaultSubobject<USphereComponent>(TEXT("OxygenSphere"));
	OxygenSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	RootComponent = OxygenSphere;

	ScrubberProgression.BaseValue        = 500.0f; 
	ScrubberProgression.AdditivePerLevel = 1500.0f; 

	ScannerProgression.BaseValue         = 0.0f; 
	ScannerProgression.AdditivePerLevel  = 1000.0f; 

	DampenerProgression.BaseValue           = 1.0f; 
	DampenerProgression.AdditivePerLevel    = 0.0f;
	DampenerProgression.MultiplierPerLevel  = 0.95f;

	RetrieverProgression.BaseValue          = 0.0f;
	RetrieverProgression.AdditivePerLevel   = 5.0f;

	RefinerProgression.BaseValue            = 1.0f;
	RefinerProgression.AdditivePerLevel     = 0.05f;

	OxygenRegenRadius = ScrubberProgression.BaseValue;
	OxygenSphere->InitSphereRadius(OxygenRegenRadius);

	OxygenSphere->OnComponentBeginOverlap.AddDynamic(this, &ABaseCamp::OnOverlapBegin);
	OxygenSphere->OnComponentEndOverlap.AddDynamic(this,   &ABaseCamp::OnOverlapEnd);
}

// ─────────────────────────────────────────────────────────────────────────────
// ENGINE OVERRIDES — unchanged
// ─────────────────────────────────────────────────────────────────────────────

void ABaseCamp::BeginPlay()
{
	Super::BeginPlay();
	
	ApplyShipUpgrades(); 
	
	TArray<AActor*> OverlappingActors;
	OxygenSphere->GetOverlappingActors(OverlappingActors, AAlphaExilemetCharacter::StaticClass());

	for (AActor* Actor : OverlappingActors)
	{
		if (AAlphaExilemetCharacter* Character = Cast<AAlphaExilemetCharacter>(Actor))
		{
			Character->EnterSafeZone();
			// Also fire the delegate for any listeners (e.g. ShipActor).
			// Note: ShipActor may not have bound yet at this point because
			// BeginPlay order isn't guaranteed. ShipActor handles this edge
			// case with its own initial state (bCurrentlyOpen = false, which
			// is correct — ship starts closed until the player walks in).
			OnPlayerEnteredCamp.Broadcast();
		}
	}
}

void ABaseCamp::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	
	if (OxygenSphere)
		OxygenSphere->SetSphereRadius(OxygenRegenRadius);

	BP_UpdateForcefieldRadius(OxygenRegenRadius);
}

// ─────────────────────────────────────────────────────────────────────────────
// SHIP REPAIR PROGRESSION — unchanged
// ─────────────────────────────────────────────────────────────────────────────

int32 ABaseCamp::GetShipSystemLevel(EShipSystem SystemID)
{
	if (ShipRepairLevels.Contains(SystemID))
		return ShipRepairLevels[SystemID];
	return 0; 
}

void ABaseCamp::UpgradeShipSystem(EShipSystem SystemToUpgrade)
{
	if (!ShipRepairLevels.Contains(SystemToUpgrade))
		ShipRepairLevels.Add(SystemToUpgrade, 0);

	if (ShipRepairLevels[SystemToUpgrade] < 5)
		ShipRepairLevels[SystemToUpgrade]++;

	ApplyShipUpgrades();
}

void ABaseCamp::ApplyShipUpgrades()
{
	int32 ScrubberLevel  = GetShipSystemLevel(EShipSystem::AtmosphericScrubber);
	OxygenRegenRadius    = ScrubberProgression.GetValueAtLevel(ScrubberLevel);
	
	if (OxygenSphere)
		OxygenSphere->SetSphereRadius(OxygenRegenRadius);
	BP_UpdateForcefieldRadius(OxygenRegenRadius);

	int32 ScannerLevel    = GetShipSystemLevel(EShipSystem::TopographyScanner);
	float NewScannerRadius = ScannerProgression.GetValueAtLevel(ScannerLevel);

	if (AAlphaExilemetCharacter* Player = Cast<AAlphaExilemetCharacter>(
			GetWorld()->GetFirstPlayerController()->GetPawn()))
	{
		if (Player->ScannerSphere)
			Player->ScannerSphere->SetSphereRadius(NewScannerRadius);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// SHOP PROGRESSION — unchanged
// ─────────────────────────────────────────────────────────────────────────────

bool ABaseCamp::IsToolUnlocked(EToolType ToolID)        { return UnlockedTools.Contains(ToolID); }
bool ABaseCamp::IsSpecialItemUnlocked(ESpecialItem ItemID) { return UnlockedSpecialItems.Contains(ItemID); }

void ABaseCamp::UnlockTool(EToolType ToolID)
{
	if (!UnlockedTools.Contains(ToolID)) UnlockedTools.Add(ToolID); 
}

void ABaseCamp::UnlockSpecialItem(ESpecialItem ItemID)
{
	if (!UnlockedSpecialItems.Contains(ItemID)) UnlockedSpecialItems.Add(ItemID);
}

// ─────────────────────────────────────────────────────────────────────────────
// EVENT HANDLERS
// Only change: broadcast delegates AFTER the character call, so the character
// state is correct before any listener reacts.
// ─────────────────────────────────────────────────────────────────────────────

void ABaseCamp::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (AAlphaExilemetCharacter* Character = Cast<AAlphaExilemetCharacter>(OtherActor))
	{
		if (OtherComp == Character->GetCapsuleComponent())
		{
			Character->EnterSafeZone();
			OnPlayerEnteredCamp.Broadcast();   // ← new: ShipActor reacts here
		}
	}
}

void ABaseCamp::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (AAlphaExilemetCharacter* Character = Cast<AAlphaExilemetCharacter>(OtherActor))
	{
		if (OtherComp == Character->GetCapsuleComponent())
		{
			Character->ExitSafeZone();
			OnPlayerExitedCamp.Broadcast();    // ← new: ShipActor reacts here
		}
	}
}