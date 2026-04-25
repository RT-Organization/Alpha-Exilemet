#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AlphaExilemet/Tools/ToolBase.h"
#include "SellTerminalWidget.generated.h"

// Struct to hold the data of a checked item
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

	// Overload so we can easily find/remove specific sales
	bool operator==(const FPendingSale& Other) const
	{
		return Tool == Other.Tool && ResourceID == Other.ResourceID;
	}
};

UCLASS()
class ALPHAEXILEMET_API USellTerminalWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Tracks everything the player has checked
	UPROPERTY(BlueprintReadWrite, Category = "Terminal")
	TArray<FPendingSale> PendingSales;

	UPROPERTY(BlueprintReadWrite, Category = "Terminal")
	int32 SelectedCreditsTotal;

	// The UI calls this when a checkbox is toggled
	UFUNCTION(BlueprintCallable, Category = "Terminal")
	void ToggleSaleItem(AToolBase* Tool, FName ResourceID, int32 Amount, int32 CreditValue, bool bIsSelected);

	// The UI calls this when "Sell Selected" is clicked
	UFUNCTION(BlueprintCallable, Category = "Terminal")
	void ProcessPendingSales(class AAlphaExilemetCharacter* Player);

	UFUNCTION(BlueprintCallable, Category = "Terminal")
	void ClearPendingSales();
};