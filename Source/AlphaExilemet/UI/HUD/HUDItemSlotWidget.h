#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Texture2D.h"
#include "HUDItemSlotWidget.generated.h"

// ─────────────────────────────────────────────────────────────────────────────
// FHUDItemSlotData
// One entry per display slot (Pickaxe ore slot OR GasRod sphere slot).
// Built by UHUDToolInventoryWidget::GetItemSlotDataArray().
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct FHUDItemSlotData
{
	GENERATED_BODY()

	// Row name in DT_Resources — empty if slot is unused/locked
	UPROPERTY(BlueprintReadOnly, Category = "HUDItemSlot")
	FName ResourceID;

	// Amount of this resource stored in this slot
	UPROPERTY(BlueprintReadOnly, Category = "HUDItemSlot")
	int32 Amount = 0;

	// Icon loaded from DT_Resources
	UPROPERTY(BlueprintReadOnly, Category = "HUDItemSlot")
	UTexture2D* Icon = nullptr;

	// Primary color from DT_Resources
	UPROPERTY(BlueprintReadOnly, Category = "HUDItemSlot")
	FLinearColor PrimaryColor = FLinearColor::White;

	// True when Amount > 0
	UPROPERTY(BlueprintReadOnly, Category = "HUDItemSlot")
	bool bIsOccupied = false;

	// True when the slot index is beyond the tool's current capacity upgrade —
	// render it as greyed-out / locked in the HUD.
	UPROPERTY(BlueprintReadOnly, Category = "HUDItemSlot")
	bool bIsLocked = false;
};

// ─────────────────────────────────────────────────────────────────────────────
// UHUDItemSlotWidget
// C++ parent for WBP_HUDItemSlot.
// Receives one FHUDItemSlotData and fires BP_OnSlotDataSet so the Blueprint
// can update its visuals (icon, amount text, locked state, etc.).
// ─────────────────────────────────────────────────────────────────────────────
UCLASS(Abstract, Blueprintable)
class ALPHAEXILEMET_API UHUDItemSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Called by WBP_HUDToolInventory when building or refreshing slots.
	UFUNCTION(BlueprintCallable, Category = "HUD|ItemSlot")
	void InitHUDSlot(const FHUDItemSlotData& SlotData);

	// The last data set on this slot — read from Blueprint if needed.
	UPROPERTY(BlueprintReadOnly, Category = "HUD|ItemSlot")
	FHUDItemSlotData CachedSlotData;

protected:
	// Implement in WBP_HUDItemSlot:
	//   • Set Icon_Item texture (Icon field)
	//   • Set Text_Amount text  (Amount field, hidden if Amount == 0)
	//   • If bIsLocked  → show locked overlay, hide content
	//   • If bIsOccupied && !bIsLocked → show normal occupied style
	//   • Else           → show empty style
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|ItemSlot")
	void BP_OnSlotDataSet(const FHUDItemSlotData& SlotData);
};
