#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "AlphaExilemetTypes.generated.h"

// USTRUCT(BlueprintType) makes this struct usable and visible inside Blueprints
USTRUCT(BlueprintType)
struct FUpgradeCost
{
	GENERATED_BODY()

public:
	// The image to display in the UI Panel
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade Data")
	UTexture2D* Icon;

	// The name of the item or upgrade (e.g., "Health Level 2", "Pickaxe")
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade Data")
	FName ItemName;

	// How much money this costs
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade Data")
	int32 CurrencyCost;

	// A dictionary of materials needed. The FName is the material name (e.g., "Slime", "Gas"), 
	// and the int32 is the amount required.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade Data")
	TMap<FName, int32> RequiredMaterials;

	// Default constructor
	FUpgradeCost()
	{
		Icon = nullptr;
		ItemName = NAME_None;
		CurrencyCost = 0;
	}
};