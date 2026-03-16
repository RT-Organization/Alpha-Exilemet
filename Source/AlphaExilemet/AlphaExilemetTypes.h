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
	Hull,
	Armor,
	OxygenSystem,
	Engine,
	Thrusters
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

	// (The Row Name itself will be MAT00, SL01, etc.)
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Data")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Data", meta=(MultiLine="true"))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Data")
	UTexture2D* Icon = nullptr;

	// How many credits this sells for at the Sell Terminal
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Data")
	int32 SellValue = 0; 
};

// 4. SHIP REPAIR TABLE (For the Ship Terminal)
USTRUCT(BlueprintType)
struct FShipRepairRow : public FTableRowBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship Repair")
	EShipSystem SystemID = EShipSystem::Hull;

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