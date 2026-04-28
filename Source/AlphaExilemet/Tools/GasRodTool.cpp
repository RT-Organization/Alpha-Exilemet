#include "GasRodTool.h"
#include "AlphaExilemet/Core/AlphaExilemetSaveGame.h"

AGasRodTool::AGasRodTool()
{
	PrimaryActorTick.bCanEverTick = true;

	// Total sphere slots. Base = 4 slots, +2 per upgrade level.
	CapacityProgression.BaseValue        = 4.0f;
	CapacityProgression.AdditivePerLevel = 2.0f;

	AbsSpeedProgression.BaseValue        = 10.0f;
	AbsSpeedProgression.AdditivePerLevel = 2.0f;

	RangeProgression.BaseValue           = 1000.0f;
	RangeProgression.AdditivePerLevel    = 200.0f;
}

void AGasRodTool::BeginPlay()
{
	Super::BeginPlay();
}

void AGasRodTool::StartUsing_Implementation()
{
	Super::StartUsing_Implementation();
	// TODO: Fire an empty sphere toward the aimed gas cloud.
	// Call TakeEmptySphere() here, then spawn the projectile BP.
}

void AGasRodTool::StopUsing_Implementation()
{
	Super::StopUsing_Implementation();
}

// =========================================================================
// STAT GETTERS
// =========================================================================

float AGasRodTool::GetAbsorptionSpeed() const
{
	int32 Level = ToolUpgradeLevels.FindRef(FName("Rod_AbsSpeed"));
	return AbsSpeedProgression.GetValueAtLevel(Level);
}

float AGasRodTool::GetRodRange() const
{
	int32 Level = ToolUpgradeLevels.FindRef(FName("Rod_Distance"));
	return RangeProgression.GetValueAtLevel(Level);
}

float AGasRodTool::GetMaxCapacity() const
{
	int32 Level = ToolUpgradeLevels.FindRef(FName("Rod_Quantity"));
	return CapacityProgression.GetValueAtLevel(Level);
}

// =========================================================================
// SPHERE OPERATIONS
// =========================================================================

int32 AGasRodTool::AddEmptySpheresToRod(int32 Quantity)
{
	if (Quantity <= 0) return 0;

	int32 MaxSlots   = FMath::FloorToInt(GetMaxCapacity());
	int32 TotalInRod = GetTotalSpheresInRod(); // empty + full
	int32 FreeSlots  = FMath::Max(0, MaxSlots - TotalInRod);
	int32 ToAdd      = FMath::Min(Quantity, FreeSlots);

	EmptySphereCount += ToAdd;
	return ToAdd;
}

bool AGasRodTool::TakeEmptySphere()
{
	if (EmptySphereCount <= 0) return false;
	EmptySphereCount--;
	return true;
}

bool AGasRodTool::ReturnFullSphere(FName GasType)
{
	if (IsRodFull()) return false;
	HarvestedGas.FindOrAdd(GasType)++;
	return true;
}

// =========================================================================
// INVENTORY QUERIES
// =========================================================================

int32 AGasRodTool::GetTotalSpheresInRod() const
{
	int32 FullSpheres = 0;
	for (const auto& Pair : HarvestedGas) FullSpheres += Pair.Value;
	return EmptySphereCount + FullSpheres;
}

bool AGasRodTool::CanFireSphere() const
{
	return EmptySphereCount > 0;
}

bool AGasRodTool::IsRodFull() const
{
	return GetTotalSpheresInRod() >= FMath::FloorToInt(GetMaxCapacity());
}

// =========================================================================
// INVENTORY — used by the upgrade cost system & inspect widget
// =========================================================================

void AGasRodTool::ClearInventory(float RetainedFraction)
{
	EmptySphereCount = 0; // Lost on death — empty spheres are physical items

	if (RetainedFraction <= 0.0f)
	{
		HarvestedGas.Empty();
		return;
	}

	for (auto It = HarvestedGas.CreateIterator(); It; ++It)
	{
		int32 Retained = FMath::FloorToInt(It.Value() * RetainedFraction);
		if (Retained > 0) It.Value() = Retained;
		else              It.RemoveCurrent();
	}
}

TMap<FName, int32> AGasRodTool::GetAllResources() const
{
	// Returns FULL spheres only. Empty spheres are not a "resource" for upgrade costs.
	return HarvestedGas;
}

int32 AGasRodTool::RemoveResource(FName InResourceID, int32 Amount)
{
	if (Amount <= 0 || !HarvestedGas.Contains(InResourceID)) return 0;

	int32 Current  = HarvestedGas[InResourceID];
	int32 ToRemove = FMath::Min(Current, Amount);

	HarvestedGas[InResourceID] -= ToRemove;

	if (HarvestedGas[InResourceID] <= 0)
		HarvestedGas.Remove(InResourceID);

	// Each removed full sphere becomes an empty sphere again
	// (the gas was consumed but the physical sphere remains in the rod)
	EmptySphereCount += ToRemove;

	return ToRemove;
}

int32 AGasRodTool::GetResourceAmount(FName InResourceID) const
{
	return HarvestedGas.Contains(InResourceID) ? HarvestedGas[InResourceID] : 0;
}

// =========================================================================
// SAVE & LOAD
// =========================================================================

void AGasRodTool::SaveToolData(UAlphaExilemetSaveGame* SaveObject)
{
	Super::SaveToolData(SaveObject);
	if (!SaveObject) return;

	SaveObject->SavedHarvestedGas       = HarvestedGas;
	SaveObject->SavedEmptySphereCount   = EmptySphereCount;
}

void AGasRodTool::LoadToolData(UAlphaExilemetSaveGame* SaveObject)
{
	Super::LoadToolData(SaveObject);
	if (!SaveObject) return;

	HarvestedGas     = SaveObject->SavedHarvestedGas;
	EmptySphereCount = SaveObject->SavedEmptySphereCount;
}
