#include "GasRodTool.h"
#include "AlphaExilemet/Core/AlphaExilemetSaveGame.h"

AGasRodTool::AGasRodTool()
{
	PrimaryActorTick.bCanEverTick = true;

	CapacityProgression.BaseValue        = 1.0f;
	CapacityProgression.AdditivePerLevel  = 1.0f;

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
	
	if (HarvestedGas.Contains(Key))
	{
		HarvestedGas[Key] += Quantity;
		return true;
	}
	
	int32 MaxSlots   = FMath::FloorToInt(GetMaxCapacity());
	int32 CurrentSlots = HarvestedGas.Num();
	
	if (CurrentSlots >= MaxSlots)
	{
		return false;
	}
	
	HarvestedGas.Add(Key, Quantity);
	return true;
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
