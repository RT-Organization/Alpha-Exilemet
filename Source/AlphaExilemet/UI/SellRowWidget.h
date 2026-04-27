#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AlphaExilemet/Data/AlphaExilemetTypes.h"
#include "Engine/DataTable.h"
#include "SellRowWidget.generated.h"

class AToolBase;
class USellTerminalWidget;

USTRUCT(BlueprintType)
struct FSellSlotData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "SellSlot")
	FName ResourceID;

	UPROPERTY(BlueprintReadOnly, Category = "SellSlot")
	int32 Amount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "SellSlot")
	int32 CreditValue = 0;

	UPROPERTY(BlueprintReadOnly, Category = "SellSlot")
	UTexture2D* Icon = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "SellSlot")
	FLinearColor PrimaryColor = FLinearColor::White;

	UPROPERTY(BlueprintReadOnly, Category = "SellSlot")
	FLinearColor SecondaryColor = FLinearColor::White;

	// Amount > 0 and slot is within CurrentCapacity
	UPROPERTY(BlueprintReadOnly, Category = "SellSlot")
	bool bIsOccupied = false;

	// Slot index >= CurrentCapacity (not yet unlocked via upgrade)
	UPROPERTY(BlueprintReadOnly, Category = "SellSlot")
	bool bIsLocked = false;
};

UCLASS(Abstract, Blueprintable)
class ALPHAEXILEMET_API USellRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	// ─── SETUP ─────────────────────────────────────────────────────────────
	// InCurrentCapacity  = actual usable slots at the player's current upgrade level
	// InMaxDisplaySlots  = total slots to always show (e.g. 6 = max upgrade level)
	// Slots 0..InCurrentCapacity-1  → usable (occupied or empty)
	// Slots InCurrentCapacity..InMaxDisplaySlots-1 → locked (upgrade required)
	// Fires BP_OnRowDataReady when done so the BP can build the UI.
	UFUNCTION(BlueprintCallable, Category = "SellRow|Setup")
	void InitSellRow(AToolBase* InTool,
	                 int32 InCurrentCapacity,
	                 int32 InMaxDisplaySlots,
	                 UDataTable* InResourceDataTable);

	// ─── SLOT DATA ─────────────────────────────────────────────────────────
	// Returns InMaxDisplaySlots entries.
	// Locked slots have bIsLocked=true and empty ResourceID.
	UFUNCTION(BlueprintPure, Category = "SellRow|Slots")
	TArray<FSellSlotData> BuildSlotDataArray() const;

	// ─── ECONOMY ───────────────────────────────────────────────────────────
	UFUNCTION(BlueprintPure, Category = "SellRow|Economy")
	bool HasAnyItems() const;

	UFUNCTION(BlueprintPure, Category = "SellRow|Economy")
	int32 GetRowTotalCreditValue() const;

	// ─── SELECTION ─────────────────────────────────────────────────────────
	UFUNCTION(BlueprintCallable, Category = "SellRow|Selection")
	void SetAllSlotsSelected(bool bSelect, USellTerminalWidget* Terminal);

	UFUNCTION(BlueprintCallable, Category = "SellRow|Selection")
	void RemoveAllFromPendingSales(USellTerminalWidget* Terminal);

	// ─── BLUEPRINT EVENTS ──────────────────────────────────────────────────
	// Fired at the end of InitSellRow. Override in WBP_SellRow /
	// WBP_SellRow_Vacuum to call SetBasicComponents → PopulateSlots.
	UFUNCTION(BlueprintImplementableEvent, Category = "SellRow|Events")
	void BP_OnRowDataReady();

	UFUNCTION(BlueprintImplementableEvent, Category = "SellRow|Events")
	void BP_SetAllSlotsVisualState(bool bSelected);

	// ─── DATA ──────────────────────────────────────────────────────────────
	UPROPERTY(BlueprintReadWrite, Category = "SellRow|Runtime")
	AToolBase* ToolReference = nullptr;

	// Actual usable capacity at the current upgrade level
	UPROPERTY(BlueprintReadWrite, Category = "SellRow|Runtime")
	int32 CurrentCapacity = 0;

	// Always equals the max possible slots (e.g. 6) — drives the total display
	UPROPERTY(BlueprintReadWrite, Category = "SellRow|Runtime")
	int32 MaxDisplaySlots = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SellRow|Setup")
	UDataTable* ResourceDataTable = nullptr;

	// Cached terminal reference — set from SpawningSequence BEFORE InitSellRow.
	// Wire SetAllSlotsSelected → Terminal to this variable.
	UPROPERTY(BlueprintReadWrite, Category = "SellRow|Runtime")
	USellTerminalWidget* TerminalRef = nullptr;

protected:
	bool GetRawInventoryFromTool(TMap<FName, int32>& OutMap) const;
	void HydrateSlotFromTable(FSellSlotData& OutSlotData) const;
};
