#pragma once

#include "CoreMinimal.h"
#include "SellRowWidget.h"
#include "VacuumSellRowWidget.generated.h"

class AVacuumTool;
class USellTerminalWidget;

// ─────────────────────────────────────────────────────────────────────────────
// Per-segment data for the vacuum bar.
// Each entry represents ONE slime type inside the tank.
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct FVacuumSegmentData
{
	GENERATED_BODY()

	// Row name in DT_Resources (e.g. "SL01")
	UPROPERTY(BlueprintReadOnly, Category = "VacuumSegment")
	FName ResourceID;

	// Raw units stored for this slime type (e.g. 15)
	UPROPERTY(BlueprintReadOnly, Category = "VacuumSegment")
	int32 Amount = 0;

	// Amount / MaxTankCapacity — use this to drive Set Size on the HBox slot
	// e.g. 15 units / 100 max = 0.15  → segment fills 15% of the bar
	UPROPERTY(BlueprintReadOnly, Category = "VacuumSegment")
	float FillRatio = 0.0f;

	// Amount * SellValue from DT_Resources
	UPROPERTY(BlueprintReadOnly, Category = "VacuumSegment")
	int32 CreditValue = 0;

	UPROPERTY(BlueprintReadOnly, Category = "VacuumSegment")
	FLinearColor PrimaryColor = FLinearColor::White;

	UPROPERTY(BlueprintReadOnly, Category = "VacuumSegment")
	FLinearColor SecondaryColor = FLinearColor::White;

	UPROPERTY(BlueprintReadOnly, Category = "VacuumSegment")
	bool bIsEmpty = false;
};

// ─────────────────────────────────────────────────────────────────────────────
// UVacuumSellRowWidget — C++ parent for WBP_SellRow_Vacuum
//
// Extends USellRowWidget with vacuum-specific segment logic.
// The base class still handles: ToolReference, TerminalRef,
// SetAllSlotsSelected, HasAnyItems, GetRowTotalCreditValue, BP_OnRowDataReady.
// ─────────────────────────────────────────────────────────────────────────────
UCLASS(Abstract, Blueprintable)
class ALPHAEXILEMET_API UVacuumSellRowWidget : public USellRowWidget
{
	GENERATED_BODY()

public:
	// Dedicated setup for the Vacuum row, bypassing the slot-based logic of the base class.
	UFUNCTION(BlueprintCallable, Category = "Vacuum|Setup")
	void InitVacuumSellRow(AVacuumTool* InVacuumTool, int32 InTankCapacity, UDataTable* InResourceDataTable);

	// ─── SEGMENT DATA ──────────────────────────────────────────────────────

	// Returns one FVacuumSegmentData per slime type currently stored.
	// FillRatio = Amount / MaxTankCapacity (float 0..1) — drives bar width.
	// Call this in PopulateSlimeSegments to create and init each segment widget.
	UFUNCTION(BlueprintPure, Category = "Vacuum|Segments")
	TArray<FVacuumSegmentData> BuildSegmentDataArray() const;

	// ─── TANK INFO ─────────────────────────────────────────────────────────

	// Total units stored across all slime types
	UFUNCTION(BlueprintPure, Category = "Vacuum|Tank")
	int32 GetTotalStoredSlime() const;

	// 0.0–1.0 fill ratio of the entire tank
	UFUNCTION(BlueprintPure, Category = "Vacuum|Tank")
	float GetTankFillRatio() const;

	// True if the tank has any slime at all — used to show/hide the row checkbox
	// Override of HasAnyItems from base class (vacuum-specific logic)
	UFUNCTION(BlueprintPure, Category = "Vacuum|Tank")
	bool VacuumHasAnySlime() const;

private:
	// Helper to get the vacuum tool cast
	AVacuumTool* GetVacuumTool() const;

	// Looks up DT_Resources and fills presentation fields on a segment entry
	void HydrateSegmentFromTable(FVacuumSegmentData& OutSeg) const;
};
