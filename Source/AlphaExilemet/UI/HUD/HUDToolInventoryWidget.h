#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AlphaExilemet/Data/AlphaExilemetTypes.h"
#include "HUDItemSlotWidget.h"
#include "HUDVacuumSegmentWidget.h"
#include "HUDToolInventoryWidget.generated.h"

class AToolBase;
class AAlphaExilemetCharacter;
class AVacuumTool;

// ─────────────────────────────────────────────────────────────────────────────
// EHUDInventoryType
// Tells the Blueprint which panel to show (slot grid vs. vacuum tank bar).
// ─────────────────────────────────────────────────────────────────────────────
UENUM(BlueprintType)
enum class EHUDInventoryType : uint8
{
	None        UMETA(DisplayName = "None"),
	ItemSlots   UMETA(DisplayName = "Item Slots (Pickaxe / GasRod)"),
	VacuumTank  UMETA(DisplayName = "Vacuum Tank")
};

// ─────────────────────────────────────────────────────────────────────────────
// UHUDToolInventoryWidget
// C++ parent for WBP_HUDToolInventory.
//
// RESPONSIBILITIES:
//   • Manages the tool-switch animation flow (hide → swap → show).
//   • Builds and caches slot/segment data arrays from the active tool.
//   • Auto-refreshes every AutoRefreshInterval seconds so the HUD stays live.
//   • Exposes simple Blueprint events so all visual logic stays in WBP.
//
// TOOL-SWITCH ANIMATION FLOW:
//   1. WBP_PlayerHUD receives OnToolWielded → calls NotifyToolChanged(NewTool).
//   2. C++ fires BP_PlayHideAnimation()  (slide-down animation in BP).
//   3. BP animation notifies end → calls OnHideAnimationFinished().
//   4. C++ swaps tool, rebuilds data, fires BP_OnInventoryRebuilt() then
//      BP_PlayShowAnimation() (slide-up animation in BP).
// ─────────────────────────────────────────────────────────────────────────────
UCLASS(Abstract, Blueprintable)
class ALPHAEXILEMET_API UHUDToolInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// =========================================================================
	// SETUP
	// =========================================================================

	// Call once from WBP_PlayerHUD's Event Construct.
	// Stores the player reference and starts the auto-refresh timer.
	UFUNCTION(BlueprintCallable, Category = "HUD|ToolInventory")
	void InitWithPlayer(AAlphaExilemetCharacter* InPlayer);

	// =========================================================================
	// TOOL SWITCH FLOW
	// =========================================================================

	// Called by WBP_PlayerHUD when OnToolWielded fires (pass the new CurrentTool,
	// or nullptr when the player holsters without equipping another tool).
	UFUNCTION(BlueprintCallable, Category = "HUD|ToolInventory")
	void NotifyToolChanged(AToolBase* NewTool);

	// *** Call this from Blueprint at the END of the hide (slide-down) animation. ***
	// It swaps CurrentTool → PendingTool, rebuilds all data arrays,
	// fires BP_OnInventoryRebuilt, then fires BP_PlayShowAnimation.
	UFUNCTION(BlueprintCallable, Category = "HUD|ToolInventory")
	void OnHideAnimationFinished();

	// =========================================================================
	// LIVE REFRESH
	// =========================================================================

	// Rebuilds slot/segment data from the current tool and fires BP_OnInventoryRebuilt.
	// Called automatically by the internal timer (AutoRefreshInterval).
	// Also safe to call manually from Blueprint (e.g. from InventoryChanged event
	// on PickaxeTool or after the vacuum absorbs slime).
	UFUNCTION(BlueprintCallable, Category = "HUD|ToolInventory")
	void RefreshInventoryDisplay();

	// =========================================================================
	// DATA QUERIES  (call from Blueprint inside BP_OnInventoryRebuilt)
	// =========================================================================

	// Returns ItemSlots or VacuumTank so the BP knows which panel to show.
	UFUNCTION(BlueprintPure, Category = "HUD|ToolInventory")
	EHUDInventoryType GetCurrentInventoryType() const;

	// ── Pickaxe / GasRod ──────────────────────────────────────────────────────
	// Returns one FHUDItemSlotData per slot up to GetCurrentCapacity().
	// Locked slots are NOT included — only slots the tool can actually use.
	UFUNCTION(BlueprintPure, Category = "HUD|ToolInventory")
	TArray<FHUDItemSlotData> GetItemSlotDataArray() const;

	// ── Vacuum ────────────────────────────────────────────────────────────────
	// Returns one FHUDVacuumSegmentData per slime type + one trailing empty segment.
	UFUNCTION(BlueprintPure, Category = "HUD|ToolInventory")
	TArray<FHUDVacuumSegmentData> GetVacuumSegmentDataArray() const;

	// ── Shared ────────────────────────────────────────────────────────────────
	// For ItemSlots: number of usable slot widgets to spawn.
	// For VacuumTank: tank capacity in units (used to size the tank background).
	UFUNCTION(BlueprintPure, Category = "HUD|ToolInventory")
	int32 GetCurrentCapacity() const;

	// Vacuum-only helpers (safe to call for other tool types — return 0/0.0).
	UFUNCTION(BlueprintPure, Category = "HUD|ToolInventory")
	int32 GetVacuumCurrentAmount() const;

	UFUNCTION(BlueprintPure, Category = "HUD|ToolInventory")
	float GetVacuumFillPercent() const;

	// Header info for the row label.
	UFUNCTION(BlueprintPure, Category = "HUD|ToolInventory")
	FText GetCurrentToolDisplayName() const;

	UFUNCTION(BlueprintPure, Category = "HUD|ToolInventory")
	UTexture2D* GetCurrentToolIcon() const;

	// =========================================================================
	// CONFIGURATION  (set in WBP_HUDToolInventory Class Defaults)
	// =========================================================================

	// Assign DT_Resources here so C++ can look up icons and colors.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|ToolInventory|Setup")
	UDataTable* ResourceDataTable;

	// Widget classes for dynamic spawning inside BP_OnInventoryRebuilt.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|ToolInventory|Setup")
	TSubclassOf<UHUDItemSlotWidget> ItemSlotWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|ToolInventory|Setup")
	TSubclassOf<UHUDVacuumSegmentWidget> VacuumSegmentWidgetClass;

	// Seconds between automatic data refreshes while a tool is equipped.
	// 0.2 is a good default — feels instant but doesn't run every frame.
	// Set to 0 to disable auto-refresh (you must call RefreshInventoryDisplay manually).
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HUD|ToolInventory|Setup")
	float AutoRefreshInterval = 0.2f;

protected:
	// =========================================================================
	// BLUEPRINT EVENTS  (implement in WBP_HUDToolInventory)
	// =========================================================================

	// Fired when a new tool is coming — play your slide-DOWN animation here.
	// When the animation ends, call OnHideAnimationFinished().
	// If there is no current tool (first equip at game start), this is NOT fired;
	// OnHideAnimationFinished() is called immediately instead.
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|ToolInventory|Events")
	void BP_PlayHideAnimation();

	// Fired after data is rebuilt and the widget is ready for display.
	// Populate your slot/segment containers HERE (clear children + spawn widgets).
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|ToolInventory|Events")
	void BP_OnInventoryRebuilt();

	// Fired after BP_OnInventoryRebuilt — play your slide-UP animation here.
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|ToolInventory|Events")
	void BP_PlayShowAnimation();

	// Fired when no tool is equipped (holster with nothing pending).
	// Hide the entire inventory panel here.
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|ToolInventory|Events")
	void BP_OnNoToolEquipped();

private:
	// =========================================================================
	// INTERNAL STATE
	// =========================================================================

	UPROPERTY()
	AAlphaExilemetCharacter* OwnerPlayer = nullptr;

	// The tool currently displayed in the HUD.
	UPROPERTY()
	AToolBase* CurrentTool = nullptr;

	// The tool that will become current once the hide animation finishes.
	UPROPERTY()
	AToolBase* PendingTool = nullptr;

	// Prevents re-entrant switch requests during the animation.
	bool bIsSwitching = false;

	// Cached built data — returned by the query functions above.
	TArray<FHUDItemSlotData>        CachedItemSlots;
	TArray<FHUDVacuumSegmentData>   CachedVacuumSegments;
	int32                           CachedCapacity = 0;

	FTimerHandle RefreshTimerHandle;

	// =========================================================================
	// INTERNAL HELPERS
	// =========================================================================
	void RebuildItemSlots();
	void RebuildVacuumSegments();

	void HydrateItemSlotFromTable(FHUDItemSlotData& OutSlot) const;
	void HydrateVacuumSegmentFromTable(FHUDVacuumSegmentData& OutSeg) const;
};
