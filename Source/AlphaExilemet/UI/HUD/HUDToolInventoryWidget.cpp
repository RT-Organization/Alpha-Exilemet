#include "HUDToolInventoryWidget.h"
#include "AlphaExilemet/AlphaExilemetCharacter.h"
#include "AlphaExilemet/Tools/ToolBase.h"
#include "AlphaExilemet/Tools/PickaxeTool.h"
#include "AlphaExilemet/Tools/VacuumTool.h"
#include "AlphaExilemet/Tools/GasRodTool.h"
#include "AlphaExilemet/Data/AlphaExilemetTypes.h"
#include "TimerManager.h"

// =========================================================================
// SETUP
// =========================================================================

void UHUDToolInventoryWidget::InitWithPlayer(AAlphaExilemetCharacter* InPlayer)
{
	OwnerPlayer = InPlayer;

	if (AutoRefreshInterval > 0.0f && GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			RefreshTimerHandle,
			this,
			&UHUDToolInventoryWidget::RefreshInventoryDisplay,
			AutoRefreshInterval,
			true   // looping
		);
	}
}

// =========================================================================
// TOOL SWITCH FLOW
// =========================================================================

void UHUDToolInventoryWidget::NotifyToolChanged(AToolBase* NewTool)
{
	PendingTool  = NewTool;
	bIsSwitching = true;

	if (CurrentTool)
	{
		// There is something on screen — hide it first, then swap.
		BP_PlayHideAnimation();
	}
	else
	{
		// Nothing to hide (first equip at game start) — go straight to rebuild.
		OnHideAnimationFinished();
	}
}

void UHUDToolInventoryWidget::OnHideAnimationFinished()
{
	CurrentTool  = PendingTool;
	PendingTool  = nullptr;
	bIsSwitching = false;

	if (!CurrentTool)
	{
		BP_OnNoToolEquipped();
		return;
	}

	// Rebuild data so BP_OnInventoryRebuilt can read it immediately.
	RefreshInventoryDisplay();

	// Tell Blueprint to slide the panel back up.
	BP_PlayShowAnimation();
}

// =========================================================================
// LIVE REFRESH
// =========================================================================

void UHUDToolInventoryWidget::RefreshInventoryDisplay()
{
	// Guard: do nothing while mid-switch (data will be rebuilt in OnHideAnimationFinished).
	if (!CurrentTool || bIsSwitching) return;

	switch (GetCurrentInventoryType())
	{
	case EHUDInventoryType::ItemSlots:
		RebuildItemSlots();
		break;
	case EHUDInventoryType::VacuumTank:
		RebuildVacuumSegments();
		break;
	default:
		break;
	}

	BP_OnInventoryRebuilt();
}

// =========================================================================
// DATA QUERIES
// =========================================================================

EHUDInventoryType UHUDToolInventoryWidget::GetCurrentInventoryType() const
{
	if (!CurrentTool) return EHUDInventoryType::None;
	if (Cast<AVacuumTool>(CurrentTool))  return EHUDInventoryType::VacuumTank;
	if (Cast<APickaxeTool>(CurrentTool)) return EHUDInventoryType::ItemSlots;
	if (Cast<AGasRodTool>(CurrentTool))  return EHUDInventoryType::ItemSlots;
	return EHUDInventoryType::None;
}

TArray<FHUDItemSlotData> UHUDToolInventoryWidget::GetItemSlotDataArray() const
{
	return CachedItemSlots;
}

TArray<FHUDVacuumSegmentData> UHUDToolInventoryWidget::GetVacuumSegmentDataArray() const
{
	return CachedVacuumSegments;
}

int32 UHUDToolInventoryWidget::GetCurrentCapacity() const
{
	return CachedCapacity;
}

int32 UHUDToolInventoryWidget::GetVacuumCurrentAmount() const
{
	AVacuumTool* Vacuum = Cast<AVacuumTool>(CurrentTool);
	if (!Vacuum) return 0;

	int32 Total = 0;
	for (const auto& Pair : Vacuum->HarvestedSlime)
	{
		Total += Pair.Value;
	}
	return Total;
}

float UHUDToolInventoryWidget::GetVacuumFillPercent() const
{
	if (CachedCapacity <= 0) return 0.0f;
	return FMath::Clamp(
		static_cast<float>(GetVacuumCurrentAmount()) / static_cast<float>(CachedCapacity),
		0.0f, 1.0f);
}

FText UHUDToolInventoryWidget::GetCurrentToolDisplayName() const
{
	if (!CurrentTool) return FText::GetEmpty();
	return CurrentTool->DisplayName;
}

UTexture2D* UHUDToolInventoryWidget::GetCurrentToolIcon() const
{
	if (!CurrentTool) return nullptr;
	return CurrentTool->Icon;
}

// =========================================================================
// INTERNAL — ITEM SLOTS (Pickaxe + GasRod)
// =========================================================================

void UHUDToolInventoryWidget::RebuildItemSlots()
{
	CachedItemSlots.Empty();
	CachedCapacity = 0;

	if (!CurrentTool) return;

	// MaxCapacity is the number of usable slots at the current upgrade level.
	int32 MaxSlots = FMath::FloorToInt(CurrentTool->GetMaxCapacity());
	if (MaxSlots <= 0) MaxSlots = 1; // safety — always show at least one slot
	CachedCapacity = MaxSlots;

	// GetAllResources() returns the live inventory map regardless of tool type.
	TMap<FName, int32> RawMap = CurrentTool->GetAllResources();

	TArray<FName> Keys;
	RawMap.GetKeys(Keys);

	for (int32 i = 0; i < MaxSlots; i++)
	{
		FHUDItemSlotData OutSlot;
		OutSlot.bIsLocked = false; // We only show USABLE slots (no locked ones in HUD)

		if (i < Keys.Num())
		{
			// Slot has a resource in it
			FName Key       = Keys[i];
			OutSlot.ResourceID = Key;
			OutSlot.Amount     = RawMap[Key];
			OutSlot.bIsOccupied = (OutSlot.Amount > 0);
			HydrateItemSlotFromTable(OutSlot);
		}
		else
		{
			// Slot exists but is currently empty
			OutSlot.bIsOccupied = false;
		}

		CachedItemSlots.Add(OutSlot);
	}
}

void UHUDToolInventoryWidget::HydrateItemSlotFromTable(FHUDItemSlotData& OutSlot) const
{
	if (!ResourceDataTable || OutSlot.ResourceID.IsNone()) return;

	const FResourceRow* Row = ResourceDataTable->FindRow<FResourceRow>(
		OutSlot.ResourceID,
		TEXT("UHUDToolInventoryWidget::HydrateItemSlotFromTable"));
	if (!Row) return;

	OutSlot.Icon         = Row->Icon;
	OutSlot.PrimaryColor = Row->PrimaryColor;
}

// =========================================================================
// INTERNAL — VACUUM SEGMENTS
// =========================================================================

void UHUDToolInventoryWidget::RebuildVacuumSegments()
{
	CachedVacuumSegments.Empty();
	CachedCapacity = 0;

	AVacuumTool* Vacuum = Cast<AVacuumTool>(CurrentTool);
	if (!Vacuum) return;

	float TankMax  = FMath::Max(1.0f, Vacuum->GetMaxCapacity());
	CachedCapacity = FMath::FloorToInt(TankMax);

	float TotalFillRatio = 0.0f;

	for (const auto& Pair : Vacuum->HarvestedSlime)
	{
		if (Pair.Value <= 0) continue;

		FHUDVacuumSegmentData Seg;
		Seg.ResourceID = Pair.Key;
		Seg.Amount     = Pair.Value;
		Seg.FillRatio  = FMath::Clamp(
			static_cast<float>(Pair.Value) / TankMax,
			0.0f, 1.0f);
		Seg.bIsEmpty   = false;

		HydrateVacuumSegmentFromTable(Seg);

		TotalFillRatio += Seg.FillRatio;
		CachedVacuumSegments.Add(Seg);
	}

	// Trailing empty segment — fills whatever space is left in the tank bar
	if (TotalFillRatio < 1.0f)
	{
		FHUDVacuumSegmentData EmptySeg;
		EmptySeg.FillRatio = 1.0f - TotalFillRatio;
		EmptySeg.bIsEmpty  = true;
		CachedVacuumSegments.Add(EmptySeg);
	}
}

void UHUDToolInventoryWidget::HydrateVacuumSegmentFromTable(FHUDVacuumSegmentData& OutSeg) const
{
	if (!ResourceDataTable || OutSeg.ResourceID.IsNone()) return;

	const FResourceRow* Row = ResourceDataTable->FindRow<FResourceRow>(
		OutSeg.ResourceID,
		TEXT("UHUDToolInventoryWidget::HydrateVacuumSegmentFromTable"));
	if (!Row) return;

	OutSeg.PrimaryColor   = Row->PrimaryColor;
	OutSeg.SecondaryColor = Row->SecondaryColor;
}
