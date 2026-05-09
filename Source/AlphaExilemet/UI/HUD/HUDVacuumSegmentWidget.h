#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HUDVacuumSegmentWidget.generated.h"

// ─────────────────────────────────────────────────────────────────────────────
// FHUDVacuumSegmentData
// One entry per slime type currently stored in the tank, plus a final "empty"
// entry that fills the remaining space.
// Built by UHUDToolInventoryWidget::GetVacuumSegmentDataArray().
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct FHUDVacuumSegmentData
{
	GENERATED_BODY()

	// Row name in DT_Resources (e.g. "SL01") — empty when bIsEmpty == true
	UPROPERTY(BlueprintReadOnly, Category = "HUDVacuumSegment")
	FName ResourceID;

	// Raw units stored for this slime type
	UPROPERTY(BlueprintReadOnly, Category = "HUDVacuumSegment")
	int32 Amount = 0;

	// Amount / MaxTankCapacity  (0.0 – 1.0)
	// Use this to drive the HBox slot Size (Fill) so the segment stretches
	// proportionally inside the fixed-width tank background.
	UPROPERTY(BlueprintReadOnly, Category = "HUDVacuumSegment")
	float FillRatio = 0.0f;

	// Colors loaded from DT_Resources — use for gradient material / solid color
	UPROPERTY(BlueprintReadOnly, Category = "HUDVacuumSegment")
	FLinearColor PrimaryColor = FLinearColor::White;

	UPROPERTY(BlueprintReadOnly, Category = "HUDVacuumSegment")
	FLinearColor SecondaryColor = FLinearColor::Green;

	// True for the trailing "empty space" segment — render it as a dark/empty fill
	UPROPERTY(BlueprintReadOnly, Category = "HUDVacuumSegment")
	bool bIsEmpty = false;
};

// ─────────────────────────────────────────────────────────────────────────────
// UHUDVacuumSegmentWidget
// C++ parent for WBP_HUDVacuumSegment.
// Mirrors WBP_VacuumSegment from the Sell Terminal but is HUD-only
// (no checkbox, no click interaction).
// ─────────────────────────────────────────────────────────────────────────────
UCLASS(Abstract, Blueprintable)
class ALPHAEXILEMET_API UHUDVacuumSegmentWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Called by WBP_HUDToolInventory when populating the tank bar.
	UFUNCTION(BlueprintCallable, Category = "HUD|VacuumSegment")
	void InitHUDSegment(const FHUDVacuumSegmentData& SegData);

	// The last data set on this segment — read from Blueprint if needed.
	UPROPERTY(BlueprintReadOnly, Category = "HUD|VacuumSegment")
	FHUDVacuumSegmentData CachedSegmentData;

protected:
	// Implement in WBP_HUDVacuumSegment:
	//   • If bIsEmpty  → render as dark/transparent fill, hide percentage text
	//   • Else         → set Border_SlimeColor gradient using Primary/Secondary,
	//                    show percentage or amount text
	//   • Always set the HBox slot Size to FillRatio (Fill mode)
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|VacuumSegment")
	void BP_OnSegmentDataSet(const FHUDVacuumSegmentData& SegData);
};
