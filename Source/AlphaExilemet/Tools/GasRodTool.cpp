#include "GasRodTool.h"
#include "AlphaExilemet/Core/AlphaExilemetSaveGame.h"

AGasRodTool::AGasRodTool()
{
	PrimaryActorTick.bCanEverTick = true;

	CapacityProgression.BaseValue        = 4.0f;  // Updated: now 4 total spheres at base
	CapacityProgression.AdditivePerLevel  = 2.0f; // Updated: +2 spheres per upgrade level

	AbsSpeedProgression.BaseValue        = 10.0f;
	AbsSpeedProgression.AdditivePerLevel  = 2.0f;

	RangeProgression.BaseValue           = 1000.0f;
	RangeProgression.AdditivePerLevel     = 200.0f;
}

void AGasRodTool::BeginPlay()
{
	Super::BeginPlay();
}

void AGasRodTool::StartUsing_Implementation()
{
	Super::StartUsing_Implementation();
	// TODO: Cast the rod toward the Gas sphere and begin the absorption loop.
}

void AGasRodTool::StopUsing_Implementation()
{
	Super::StopUsing_Implementation();
	// TODO: Cancel any in-flight absorption timer.
}

/* ----------------------------- */
/* STAT GETTERS                  */
/* ----------------------------- */

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

/* ----------------------------- */
/* INVENTORY                     */
/* ----------------------------- */

void AGasRodTool::ClearInventory(float RetainedFraction)
{
	if (RetainedFraction <= 0.0f)
	{
		HarvestedGas.Empty();
		return;
	}

	for (auto It = HarvestedGas.CreateIterator(); It; ++It)
	{
		int32 RetainedAmount = FMath::FloorToInt(It.Value() * RetainedFraction);
		if (RetainedAmount > 0) It.Value() = RetainedAmount;
		else                    It.RemoveCurrent();
	}
}

bool AGasRodTool::TryAddGas(const FDataTableRowHandle& ResourceID, int32 Quantity)
{
	if (Quantity <= 0) return false;

	FName Key = ResourceID.RowName;

	int32 MaxSpheres   = FMath::FloorToInt(GetMaxCapacity()); // total sphere slots
	int32 TotalStored  = 0;
	for (const auto& Pair : HarvestedGas) TotalStored += Pair.Value;

	int32 CanAdd = FMath::Min(Quantity, MaxSpheres - TotalStored);
	if (CanAdd <= 0) return false;

	HarvestedGas.FindOrAdd(Key) += CanAdd;
	return true; // returns true even for partial adds (Quantity > CanAdd)
}

int32 AGasRodTool::GetCurrentTotalSpheres() const
{
	int32 Total = 0;
	for (const auto& Pair : HarvestedGas) Total += Pair.Value;
	return Total;
}

float AGasRodTool::GetFillPercent() const
{
	float Max = GetMaxCapacity();
	if (Max <= 0.f) return 0.f;
	return static_cast<float>(GetCurrentTotalSpheres()) / Max;
}

TMap<FName, int32> AGasRodTool::GetAllResources() const
{
	return HarvestedGas;
}

int32 AGasRodTool::RemoveResource(FName InResourceID, int32 Amount)
{
	if (Amount <= 0 || !HarvestedGas.Contains(InResourceID)) return 0;

	int32 CurrentAmount  = HarvestedGas[InResourceID];
	int32 AmountToRemove = FMath::Min(CurrentAmount, Amount);

	HarvestedGas[InResourceID] -= AmountToRemove;

	if (HarvestedGas[InResourceID] <= 0)
	{
		HarvestedGas.Remove(InResourceID);
	}

	return AmountToRemove;
}

int32 AGasRodTool::GetResourceAmount(FName InResourceID) const
{
	return HarvestedGas.Contains(InResourceID) ? HarvestedGas[InResourceID] : 0;
}

/* ----------------------------- */
/* SAVE & LOAD                   */
/* ----------------------------- */

void AGasRodTool::SaveToolData(UAlphaExilemetSaveGame* SaveObject)
{
	Super::SaveToolData(SaveObject);
	if (SaveObject) SaveObject->SavedHarvestedGas = HarvestedGas;
}

void AGasRodTool::LoadToolData(UAlphaExilemetSaveGame* SaveObject)
{
	Super::LoadToolData(SaveObject);
	if (SaveObject) HarvestedGas = SaveObject->SavedHarvestedGas;
}