#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AlphaExilemet/Data/AlphaExilemetTypes.h"
#include "Engine/DataTable.h"
#include "SellTerminalWidget.generated.h"

class AToolBase;
class AAlphaExilemetCharacter;
class APickaxeTool;
class AVacuumTool;
class AGasRodTool;

// ─────────────────────────────────────────────────────────────────────────────
// Struct that represents one item in the player's sell cart.
// Stored in PendingSales until the player confirms the transaction.
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct FPendingSale
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Sale")
	AToolBase* Tool = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Sale")
	FName ResourceID;

	UPROPERTY(BlueprintReadWrite, Category = "Sale")
	int32 Amount = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Sale")
	int32 CreditValue = 0;

	// Equality based on Tool + ResourceID so AddUnique works correctly
	bool operator==(const FPendingSale& Other) const
	{
		return Tool == Other.Tool && ResourceID == Other.ResourceID;
	}
};

// ─────────────────────────────────────────────────────────────────────────────
// USellTerminalWidget — C++ parent for WBP_SellTerminal
// ─────────────────────────────────────────────────────────────────────────────
UCLASS(Abstract, Blueprintable)
class ALPHAEXILEMET_API USellTerminalWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	// ─── DATA TABLE ────────────────────────────────────────────────────────
	// Assign DT_Resources in the WBP_SellTerminal Class Defaults.
	// Used by SellAll to look up sell values without BP.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Terminal|Setup")
	UDataTable* ResourceDataTable;

	// ─── CART STATE ────────────────────────────────────────────────────────
	UPROPERTY(BlueprintReadWrite, Category = "Terminal")
	TArray<FPendingSale> PendingSales;

	// Running total of credits for items currently in the cart.
	// Only changes when an item is actually added or removed (AddUnique guard).
	UPROPERTY(BlueprintReadOnly, Category = "Terminal")
	int32 SelectedCreditsTotal = 0;

	// ─── CART MANAGEMENT ───────────────────────────────────────────────────

	// Called by a slot/segment widget when the player checks or unchecks it.
	// BUG-FIX: SelectedCreditsTotal only updates if the item was genuinely
	// added/removed from PendingSales (AddUnique guard — fixes double-count).
	UFUNCTION(BlueprintCallable, Category = "Terminal")
	void ToggleSaleItem(AToolBase* Tool, FName ResourceID,
	                    int32 Amount, int32 CreditValue, bool bIsSelected);

	// Empties the cart and resets SelectedCreditsTotal to 0.
	UFUNCTION(BlueprintCallable, Category = "Terminal")
	void ClearPendingSales();

	// ─── SELL ──────────────────────────────────────────────────────────────

	// Sells only the items currently in the cart.
	// Removes each sold resource from its tool's inventory, pays the player,
	// then clears the cart.
	// Call this from the "SELL SELECTED" button in BP.
	UFUNCTION(BlueprintCallable, Category = "Terminal")
	void ProcessPendingSales(AAlphaExilemetCharacter* Player);

	// Sells EVERYTHING across ALL tools at once.
	// Bypasses the BP LoadRowIntoCart chain entirely (BUG-FIX for SellAll).
	// Call this from the "SELL ALL" button in BP.
	UFUNCTION(BlueprintCallable, Category = "Terminal")
	void SellAll(AAlphaExilemetCharacter* Player);

	// ─── ECONOMY HELPERS ───────────────────────────────────────────────────

	// Returns the player's current currency (for live credit display).
	UFUNCTION(BlueprintPure, Category = "Terminal")
	float GetPlayerCreditAmount(AAlphaExilemetCharacter* Player) const;

	// Returns the total raw credit value of every item across all tools.
	// Used to display the "SELL ALL X CR" label on the button.
	UFUNCTION(BlueprintPure, Category = "Terminal")
	int32 CalculateTotalValueAllTools(AAlphaExilemetCharacter* Player) const;

	// ─── BLUEPRINT EVENTS ──────────────────────────────────────────────────

	// Fired after every sell action so the BP can refresh the credits text.
	UFUNCTION(BlueprintImplementableEvent, Category = "Terminal|Events")
	void BP_OnCreditsChanged(float NewAmount);

	// Fired after SellAll or ProcessPendingSales so the BP can refresh all rows.
	UFUNCTION(BlueprintImplementableEvent, Category = "Terminal|Events")
	void BP_OnSellCompleted();

private:

	// Reads the sell value for one stack from DT_Resources.
	// Returns 0 if the DataTable or row is not found.
	int32 LookupStackValue(FName ResourceID, int32 Amount) const;

	// Collects all items from one tool into an array of FPendingSale.
	void CollectSalesFromTool(AToolBase* Tool, TArray<FPendingSale>& OutSales) const;
};
