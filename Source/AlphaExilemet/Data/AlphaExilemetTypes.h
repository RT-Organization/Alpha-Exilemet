#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "Engine/DataTable.h" // Required for FTableRowBase
#include "AlphaExilemetTypes.generated.h"

// -------------------------------------------------------------------------
// STRUCTS
// -------------------------------------------------------------------------
USTRUCT(BlueprintType)
struct FStatProgression
{
	GENERATED_BODY()

	// The starting value at Level 0
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Progression")
	float BaseValue;

	// Flat amount added per level (e.g., +20 Health)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Progression")
	float AdditivePerLevel;

	// Multiplier applied per level (e.g., 1.1 for +10%, or 0.9 for -10%)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Progression")
	float MultiplierPerLevel;

	// Default Constructor
	FStatProgression()
	{
		BaseValue = 100.0f;
		AdditivePerLevel = 0.0f;
		MultiplierPerLevel = 1.0f;
	}

	// Helper function to calculate the exact value at any given level
	float GetValueAtLevel(int32 Level) const
	{
		// Formula: (Base + (Additive * Level)) * (Multiplier ^ Level)
		float FlatTotal = BaseValue + (AdditivePerLevel * Level);
		float MultipliedTotal = FlatTotal * FMath::Pow(MultiplierPerLevel, Level);
		return MultipliedTotal;
	}
};

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
USTRUCT(BlueprintType)
struct FUpgradeCost
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade Cost")
	int32 CurrencyCost = 0;

	// Key = Row Name from your Resource Table (e.g. "MAT00", "SL01")
	// Value = amount of that resource required
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

	// Index 0 = Cost to reach Level 1, Index 4 = Cost to reach Level 5
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

	// Internal stat identifier (e.g. "Pickaxe_Strength", "Vacuum_Speed")
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool Upgrade")
	FName StatID; 

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool Upgrade")
	FText DisplayName;

	// Index 0 = Level 1, Index 4 = Level 5
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool Upgrade")
	TArray<FUpgradeCost> CostPerLevel;
};

// 3. RESOURCES TABLE
USTRUCT(BlueprintType)
struct FResourceRow : public FTableRowBase
{
	GENERATED_BODY()

	// What kind of resource is this? (Solid, Liquid, Gas)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Data")
	EResourceType ResourceType = EResourceType::Solid;

	// -------------------------------------------------------------------------
	// HEALTH
	// -------------------------------------------------------------------------
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Data")
	float Health = 100.f;

	// -------------------------------------------------------------------------
	// ECONOMY
	// -------------------------------------------------------------------------

	// How many credits this gives when sold at the Terminal.
	// Also loaded into ResourceBase::CurrencyValuePerUnit at runtime.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Data")
	int32 SellValue = 0;

	// Seconds before this vein respawns after being fully depleted.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Data",
		meta=(ClampMin="1.0", UIMin="1.0"))
	float RegenTime = 120.f;

	// -------------------------------------------------------------------------
	// PRESENTATION
	// -------------------------------------------------------------------------

	// Popup text shown to the player (where to find this resource)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Data", meta=(MultiLine="true"))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Data")
	UTexture2D* Icon = nullptr;

	// -------------------------------------------------------------------------
	// DROPS (Solid / Pickaxe only)
	// -------------------------------------------------------------------------

	// Max number of drops at Pickaxe_Luck = 0.
	// Final max = MaxDrops + Pickaxe_Luck bonus.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Data|Drops",
		meta=(EditCondition="ResourceType == EResourceType::Solid", EditConditionHides))
	int32 MaxDrops = 1;
	// Max bonus drops coming from luck (0 = no bonus from luck)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Data|Drops",
		meta=(EditCondition="ResourceType == EResourceType::Solid", EditConditionHides, ClampMin="0"))
	int32 MaxLuckBoost = 5;
	
	// -------------------------------------------------------------------------
	// VISUAL / COLOR SETTINGS
	// -------------------------------------------------------------------------

	// Primary color for Solids and Gases; starting gradient color for Slimes.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Data|Visuals")
	FLinearColor PrimaryColor = FLinearColor::White;

	// Secondary/ending gradient color. Only shown for Liquid/Slime resources.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Data|Visuals",
		meta=(EditCondition="ResourceType == EResourceType::Liquid", EditConditionHides))
	FLinearColor SecondaryColor = FLinearColor::Green;
};

// 4. SHIP REPAIR TABLE
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

	// Array length = max level (e.g. 5 entries = upgradeable to Level 5)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship Repair")
	TArray<FUpgradeCost> CostPerLevel;
};

// 5. SHOP TABLE
USTRUCT(BlueprintType)
struct FShopItemRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop Data")
	EShopItemCategory Category = EShopItemCategory::Tool;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop Data",
		meta=(EditCondition="Category == EShopItemCategory::Tool", EditConditionHides))
	EToolType ToolID = EToolType::Pickaxe;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop Data",
		meta=(EditCondition="Category == EShopItemCategory::Tool", EditConditionHides))
	TSubclassOf<class AToolBase> ToolClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop Data",
		meta=(EditCondition="Category == EShopItemCategory::SpecialItem", EditConditionHides))
	ESpecialItem SpecialItemID = ESpecialItem::TeleportBeacon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop Data")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop Data", meta=(MultiLine="true"))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop Data")
	UTexture2D* Icon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop Data")
	int32 CurrencyCost = 0; 
};