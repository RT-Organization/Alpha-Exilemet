#include "PickaxeTool.h"
#include "ResourceBase.h"
#include "SolidResource.h"

#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "TimerManager.h"
#include "AlphaExilemetSaveGame.h"

#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"

APickaxeTool::APickaxeTool()
{
	PrimaryActorTick.bCanEverTick = true;
	
	StrengthProgression.BaseValue = 8.0f;
	StrengthProgression.AdditivePerLevel = 4.0f;

	CapacityProgression.BaseValue = 1.0f;
	CapacityProgression.AdditivePerLevel = 1.0f;

	LuckProgression.BaseValue = 0.0f;
	LuckProgression.AdditivePerLevel = 5.0f;
}

void APickaxeTool::BeginPlay()
{
	Super::BeginPlay();
}

/* ----------------------------- */
/* INPUT                         */
/* ----------------------------- */

void APickaxeTool::StartUsing_Implementation()
{
	StartMiningTimer();
}
void APickaxeTool::StopUsing_Implementation()
{
	StopMiningTimer();
}

/* ----------------------------- */
/* MINING CONTROL                */
/* ----------------------------- */

void APickaxeTool::StartMiningTimer()
{
	GetWorldTimerManager().SetTimer(
		MiningTimer,
		this,
		&APickaxeTool::PerformMiningTrace,
		MiningInterval,
		true
	);
}
void APickaxeTool::StopMiningTimer()
{
	GetWorldTimerManager().ClearTimer(MiningTimer);
}

void APickaxeTool::PerformMiningTrace()
{
	if (!OwnerCharacter)
	{
		OwnerCharacter = Cast<ACharacter>(GetOwner());
		if (!OwnerCharacter) return;
	}
	
	APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController());
	if (!PC) return;
	
	APlayerCameraManager* CameraManager = PC->PlayerCameraManager;
	if (!CameraManager) return;
	
	FVector Start = CameraManager->GetCameraLocation();
	FVector Forward = CameraManager->GetCameraRotation().Vector();
	FVector End = Start + Forward * MiningRange;
	
	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(OwnerCharacter);
	
	bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
	
	if (!bHit) return;
	
	AActor* HitActor = Hit.GetActor();
	if (!HitActor) return;
	
	ApplyMiningDamage(HitActor);
}

/* ----------------------------- */
/* DAMAGE LOGIC                  */
/* ----------------------------- */

void APickaxeTool::ApplyMiningDamage(AActor* Target)
{
	if (!Target) return;
	
	ASolidResource* Solid = Cast<ASolidResource>(Target);
	if (!Solid) return;
	
	float Damage = GetMiningStrength(); 
	
	Solid->SetLastPickaxe(this);
	bool wasDepleted = Solid->ApplyResourceDamage(Damage);
}

bool APickaxeTool::TryAddOre(const FDataTableRowHandle& ResourceID, int32 Quantity)
{
	if (Quantity <= 0) return false;
	
	FName Key = ResourceID.RowName;
	
	if (HarvestedOres.Contains(Key))
	{
		HarvestedOres[Key] += Quantity;
		return true;
	}
	
	int32 MaxSlots = FMath::FloorToInt(GetMaxCapacity());
	int32 CurrentSlots = HarvestedOres.Num();
	
	if (CurrentSlots >= MaxSlots)
	{
		return false; // inventario pieno di tipi
	}
	
	HarvestedOres.Add(Key, Quantity);
	return true;
}

/* ----------------------------- */
/* STAT UPGRADES                 */
/* ----------------------------- */

float APickaxeTool::GetMiningStrength() const
{
	int32 Level = ToolUpgradeLevels.FindRef(FName("Pickaxe_Strength"));
	return StrengthProgression.GetValueAtLevel(Level);
}

float APickaxeTool::GetMiningLuck() const
{
	int32 Level = ToolUpgradeLevels.FindRef(FName("Pickaxe_Luck"));
	return LuckProgression.GetValueAtLevel(Level);
}

float APickaxeTool::GetMaxCapacity() const
{
	int32 Level = ToolUpgradeLevels.FindRef(FName("Pickaxe_Capacity"));
	return CapacityProgression.GetValueAtLevel(Level);
}

/* ----------------------------- */
/* INVENTORY                     */
/* ----------------------------- */
void APickaxeTool::ClearInventory(float RetainedFraction)
{
	if (RetainedFraction <= 0.0f)
	{
		HarvestedOres.Empty();
		return;
	}

	for (auto It = HarvestedOres.CreateIterator(); It; ++It)
	{
		int32 RetainedAmount = FMath::FloorToInt(It.Value() * RetainedFraction);
		
		if (RetainedAmount > 0)
		{
			It.Value() = RetainedAmount;
		}
		else
		{
			It.RemoveCurrent();
		}
	}
}

int32 APickaxeTool::RemoveResource(FName InResourceID, int32 Amount)
{
	if (Amount <= 0 || !HarvestedOres.Contains(InResourceID)) return 0;

	int32 CurrentAmount = HarvestedOres[InResourceID];
	int32 AmountToRemove = FMath::Min(CurrentAmount, Amount);

	HarvestedOres[InResourceID] -= AmountToRemove;

	// Clean up the map if we hit 0
	if (HarvestedOres[InResourceID] <= 0)
	{
		HarvestedOres.Remove(InResourceID);
	}

	return AmountToRemove;
}

int32 APickaxeTool::GetResourceAmount(FName InResourceID) const
{
	return HarvestedOres.Contains(InResourceID) ? HarvestedOres[InResourceID] : 0;
}

/* ----------------------------- */
/* SAVE & LOAD                   */
/* ----------------------------- */
void APickaxeTool::SaveToolData(UAlphaExilemetSaveGame* SaveObject)
{
	Super::SaveToolData(SaveObject);
	if (SaveObject) SaveObject->SavedHarvestedOres = HarvestedOres;
}

void APickaxeTool::LoadToolData(UAlphaExilemetSaveGame* SaveObject)
{
	Super::LoadToolData(SaveObject);
	if (SaveObject) HarvestedOres = SaveObject->SavedHarvestedOres;
}