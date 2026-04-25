#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AlphaExilemet/Data/AlphaExilemetTypes.h"
#include "BaseTransactionWidget.generated.h"

class AAlphaExilemetCharacter;

UCLASS()
class ALPHAEXILEMET_API UBaseTransactionWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	// Checks if the player has enough currency and materials
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Transaction")
	bool CanAfford(FUpgradeCost CostInfo, AAlphaExilemetCharacter* Player);

	// Actually subtracts the currency and materials from the player
	UFUNCTION(BlueprintCallable, Category = "AlphaExilemet|Transaction")
	void DeductCost(FUpgradeCost CostInfo, AAlphaExilemetCharacter* Player);
};