#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "Engine/DataTable.h" // Required for FTableRowBase
#include "AlphaExilemetTypes.generated.h"

// -------------------------------------------------------------------------
// ENUMS
// -------------------------------------------------------------------------

// Character Stats
UENUM(BlueprintType)
enum class EPlayerStat : uint8
{
	Health,
	Oxygen,
	Agility
};

// Tool Types
UENUM(BlueprintType)
enum class EToolType : uint8
{
	Pickaxe,
	SlimeVacuum,
	GasRod
};

// Ship repair components
UENUM(BlueprintType)
enum class EShipSystem : uint8
{
	AtmosphericScrubber,
	TopographyScanner,
	HazardDampener,
	MatterRetriever,
	MolecularRefiner
};

UENUM(BlueprintType)
enum class EShopItemCategory : uint8
{
	Tool,
	SpecialItem
};

UENUM(BlueprintType)
enum class ESpecialItem : uint8
{
	TeleportBeacon
	// You can add more here later!
};

// Defines the physical state of a resource to know WHICH tool can harvest it
UENUM(BlueprintType)
enum class EResourceType : uint8
{
	Solid    UMETA(DisplayName = "Solid (Pickaxe)"),
	Liquid   UMETA(DisplayName = "Liquid/Slime (Vacuum)"),
	Gas      UMETA(DisplayName = "Gas (Gas Rod)")
};

// -------------------------------------------------------------------------
// BASE COST STRUCT
// -------------------------------------------------------------------------
// This defines the cost for ONE level of an upgrade.
USTRUCT(BlueprintType)
struct FUpgradeCost
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade Cost")
	int32 CurrencyCost = 0;

	// The Name is the Row Name from your Resource Table (e.g., "MAT00", "SL01")
	// The int32 is the amount of that resource required.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade Cost")
	TMap<FName, int32> RequiredMaterials;
};

// -------------------------------------------------------------------------
// DATA TABLE ROW STRUCTS
// -------------------------------------------------------------------------

// 1. CHARACTER UPGRADES TABLE
USTRUCT(BlueprintType)
struct FCharacterUpgradeRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Upgrade")
	EPlayerStat StatID = EPlayerStat::Health;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Upgrade")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Upgrade", meta=(MultiLine="true"))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Upgrade")
	UTexture2D* Icon = nullptr;

	// Array of costs. Index 0 = Cost to reach Level 1. Index 4 = Cost to reach Level 5.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Upgrade")
	TArray<FUpgradeCost> CostPerLevel;
};

// 2. TOOL UPGRADES TABLE
USTRUCT(BlueprintType)
struct FToolUpgradeRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool Upgrade")
	EToolType ToolToUpgrade = EToolType::Pickaxe;

	// Identifies the specific stat internally (e.g., "Force", "Speed")
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool Upgrade")
	FName StatID; 

	// The visual name for the stat (e.g., "Absorption Speed")
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool Upgrade")
	FText DisplayName;

	// Array of costs. Index 0 = Level 1, Index 4 = Level 5.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool Upgrade")
	TArray<FUpgradeCost> CostPerLevel;
};

// 3. RESOURCES TABLE (For the Sell Terminal & General Info)
USTRUCT(BlueprintType)
struct FResourceRow : public FTableRowBase
{
	GENERATED_BODY()

	// What kind of resource is this? (Solid, Liquid, Gas)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Data")
	EResourceType ResourceType = EResourceType::Solid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Data")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Data", meta=(MultiLine="true"))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Data")
	UTexture2D* Icon = nullptr;

	// How many credits this gives when clicked "Sell" in the Terminal
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Data")
	int32 SellValue = 0; 

	// -------------------------------------------------------------------------
	// SLIME / VACUUM SPECIFIC DATA
	// -------------------------------------------------------------------------

	// How much space 1 unit of this slime takes up in the Vacuum's capacity
	// (e.g., Basic Slime = 1 space, Rare Slime = 5 spaces)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Data|Slime Settings", meta=(EditCondition="ResourceType == EResourceType::Liquid", EditConditionHides))
	int32 VolumeCost = 1;

	// The primary/starting color for the Vacuum UI Progress Bar gradient
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Data|Slime Settings", meta=(EditCondition="ResourceType == EResourceType::Liquid", EditConditionHides))
	FLinearColor PrimaryColor = FLinearColor::Green;

	// The secondary/ending color for the gradient. 
	// (Note: To make a solid color, just make this exactly the same as the PrimaryColor)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Data|Slime Settings", meta=(EditCondition="ResourceType == EResourceType::Liquid", EditConditionHides))
	FLinearColor SecondaryColor = FLinearColor::Green;
};

// 4. SHIP REPAIR TABLE (For the Ship Terminal)
USTRUCT(BlueprintType)
struct FShipRepairRow : public FTableRowBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship Repair")
	EShipSystem SystemID = EShipSystem::AtmosphericScrubber;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship Repair")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship Repair", meta=(MultiLine="true"))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship Repair")
	UTexture2D* Icon = nullptr;

	// The length of this array determines the Max Level (e.g., 3 for Hull, 6 for Thrusters)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship Repair")
	TArray<FUpgradeCost> CostPerLevel;
};

//5. SHOP TABLE
USTRUCT(BlueprintType)
struct FShopItemRow : public FTableRowBase
{
	GENERATED_BODY()

	// Tells the UI if this goes in the top row (Tools) or bottom list (Special)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop Data")
	EShopItemCategory Category = EShopItemCategory::Tool;

	// If Category is 'Tool', the UI will read this ID
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop Data", meta=(EditCondition="Category == EShopItemCategory::Tool", EditConditionHides))
	EToolType ToolID = EToolType::Pickaxe;

	// ADD THIS: The actual Blueprint class to spawn when the player buys this tool
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop Data", meta=(EditCondition="Category == EShopItemCategory::Tool", EditConditionHides))
	TSubclassOf<class AToolBase> ToolClass;

	// If Category is 'SpecialItem', the UI will read this ID
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop Data", meta=(EditCondition="Category == EShopItemCategory::SpecialItem", EditConditionHides))
	ESpecialItem SpecialItemID = ESpecialItem::TeleportBeacon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop Data")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop Data", meta=(MultiLine="true"))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop Data")
	UTexture2D* Icon = nullptr;

	// Notice: Just an int32! No materials needed for the shop.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop Data")
	int32 CurrencyCost = 0; 
};

