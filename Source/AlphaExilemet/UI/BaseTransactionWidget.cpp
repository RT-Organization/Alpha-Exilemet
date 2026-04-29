#include "BaseTransactionWidget.h"
#include "AlphaExilemet/AlphaExilemetCharacter.h"
#include "AlphaExilemet/Core/AlphaExilemetGameInstance.h"
#include "AlphaExilemet/Core/UpgradeProgressionManager.h"
#include "Kismet/GameplayStatics.h"

// ─────────────────────────────────────────────────────────────────────────────
// KEY CONVERSION
//
// UEnum::GetValueAsName() returns the full enum entry name, e.g.:
//   EPlayerStat::Health  →  "Health"
//   EShipSystem::AtmosphericScrubber  →  "AtmosphericScrubber"
//
// YOUR DATATABLE ROW NAMES MUST MATCH THESE STRINGS EXACTLY (case-sensitive).
// Check your DT row names in the DataTable editor left column.
// If you named a row "CHAR_Health" instead of "Health", either rename the DT
// row OR override the conversion here to return FName("CHAR_Health").
// ─────────────────────────────────────────────────────────────────────────────

FName UBaseTransactionWidget::StatToUpgradeKey(EPlayerStat StatID)
{
	// GetValueAsName returns the bare enum name without the prefix,
	// e.g. EPlayerStat::Health → "Health"
	const UEnum* Enum = StaticEnum<EPlayerStat>();
	if (!Enum) return NAME_None;
	// GetNameByValue returns "EPlayerStat::Health"; we want just "Health"
	FString FullName = Enum->GetNameByValue(static_cast<int64>(StatID)).ToString();
	// Strip the "EPlayerStat::" prefix
	int32 ColonIndex;
	if (FullName.FindLastChar(':', ColonIndex))
	{
		return FName(*FullName.RightChop(ColonIndex + 1));
	}
	return FName(*FullName);
}

FName UBaseTransactionWidget::ShipSystemToUpgradeKey(EShipSystem SystemID)
{
	const UEnum* Enum = StaticEnum<EShipSystem>();
	if (!Enum) return NAME_None;
	FString FullName = Enum->GetNameByValue(static_cast<int64>(SystemID)).ToString();
	int32 ColonIndex;
	if (FullName.FindLastChar(':', ColonIndex))
	{
		return FName(*FullName.RightChop(ColonIndex + 1));
	}
	return FName(*FullName);
}

// ─────────────────────────────────────────────────────────────────────────────
// PRIVATE HELPER
// ─────────────────────────────────────────────────────────────────────────────

UUpgradeProgressionManager* UBaseTransactionWidget::GetProgressionManager() const
{
	UAlphaExilemetGameInstance* GI = Cast<UAlphaExilemetGameInstance>(
		UGameplayStatics::GetGameInstance(this));
	return GI ? GI->ProgressionManager : nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
// LEGACY API
// ─────────────────────────────────────────────────────────────────────────────

bool UBaseTransactionWidget::CanAfford(FUpgradeCost CostInfo, AAlphaExilemetCharacter* Player)
{
	if (!Player) return false;
	if (Player->Currency < static_cast<float>(CostInfo.CurrencyCost)) return false;
	for (const auto& Pair : CostInfo.RequiredMaterials)
	{
		if (Player->GetTotalResourceAmount(Pair.Key) < Pair.Value) return false;
	}
	return true;
}

void UBaseTransactionWidget::DeductCost(FUpgradeCost CostInfo, AAlphaExilemetCharacter* Player)
{
	if (!Player) return;
	Player->Currency -= static_cast<float>(CostInfo.CurrencyCost);
	for (const auto& Pair : CostInfo.RequiredMaterials)
		Player->DeductResourceFromTools(Pair.Key, Pair.Value);
}

// ─────────────────────────────────────────────────────────────────────────────
// PLAYERSTAT OVERLOADS  (WBP_SystemUpgradeRow)
// ─────────────────────────────────────────────────────────────────────────────

FUpgradeCost UBaseTransactionWidget::GetRemainingCost_Stat(EPlayerStat StatID) const
{
	if (UUpgradeProgressionManager* M = GetProgressionManager())
		return M->GetCurrentCost(StatToUpgradeKey(StatID));
	return FUpgradeCost();
}

bool UBaseTransactionWidget::CanAffordFullNow_Stat(EPlayerStat StatID, AAlphaExilemetCharacter* Player) const
{
	if (UUpgradeProgressionManager* M = GetProgressionManager())
		return M->CanAffordFull(StatToUpgradeKey(StatID), Player);
	return false;
}

bool UBaseTransactionWidget::IsUpgradeAvailable_Stat(EPlayerStat StatID) const
{
	if (UUpgradeProgressionManager* M = GetProgressionManager())
		return M->HasUpgradeAvailable(StatToUpgradeKey(StatID));
	return false;
}

bool UBaseTransactionWidget::PayAndCheckComplete_Stat(EPlayerStat StatID, AAlphaExilemetCharacter* Player)
{
	if (UUpgradeProgressionManager* M = GetProgressionManager())
		return M->PayTowardsUpgrade(StatToUpgradeKey(StatID), Player);
	return false;
}

void UBaseTransactionWidget::NotifyUpgradeComplete_Stat(EPlayerStat StatID, int32 LevelJustReached)
{
	if (UUpgradeProgressionManager* M = GetProgressionManager())
		M->AdvanceToNextLevelCost(StatToUpgradeKey(StatID), LevelJustReached);
}

// ─────────────────────────────────────────────────────────────────────────────
// SHIPSYSTEM OVERLOADS  (WBP_ShipRepairRow)
// ─────────────────────────────────────────────────────────────────────────────

FUpgradeCost UBaseTransactionWidget::GetRemainingCost_Ship(EShipSystem SystemID) const
{
	if (UUpgradeProgressionManager* M = GetProgressionManager())
		return M->GetCurrentCost(ShipSystemToUpgradeKey(SystemID));
	return FUpgradeCost();
}

bool UBaseTransactionWidget::CanAffordFullNow_Ship(EShipSystem SystemID, AAlphaExilemetCharacter* Player) const
{
	if (UUpgradeProgressionManager* M = GetProgressionManager())
		return M->CanAffordFull(ShipSystemToUpgradeKey(SystemID), Player);
	return false;
}

bool UBaseTransactionWidget::IsUpgradeAvailable_Ship(EShipSystem SystemID) const
{
	if (UUpgradeProgressionManager* M = GetProgressionManager())
		return M->HasUpgradeAvailable(ShipSystemToUpgradeKey(SystemID));
	return false;
}

bool UBaseTransactionWidget::PayAndCheckComplete_Ship(EShipSystem SystemID, AAlphaExilemetCharacter* Player)
{
	if (UUpgradeProgressionManager* M = GetProgressionManager())
		return M->PayTowardsUpgrade(ShipSystemToUpgradeKey(SystemID), Player);
	return false;
}

void UBaseTransactionWidget::NotifyUpgradeComplete_Ship(EShipSystem SystemID, int32 LevelJustReached)
{
	if (UUpgradeProgressionManager* M = GetProgressionManager())
		M->AdvanceToNextLevelCost(ShipSystemToUpgradeKey(SystemID), LevelJustReached);
}

// ─────────────────────────────────────────────────────────────────────────────
// FNAME OVERLOADS  (WBP_ToolStatBlock — Cached Stat ID is already FName)
// ─────────────────────────────────────────────────────────────────────────────

FUpgradeCost UBaseTransactionWidget::GetRemainingCost(FName UpgradeKey) const
{
	if (UUpgradeProgressionManager* M = GetProgressionManager())
		return M->GetCurrentCost(UpgradeKey);
	return FUpgradeCost();
}

bool UBaseTransactionWidget::CanAffordFullNow(FName UpgradeKey, AAlphaExilemetCharacter* Player) const
{
	if (UUpgradeProgressionManager* M = GetProgressionManager())
		return M->CanAffordFull(UpgradeKey, Player);
	return false;
}

bool UBaseTransactionWidget::IsUpgradeAvailable(FName UpgradeKey) const
{
	if (UUpgradeProgressionManager* M = GetProgressionManager())
		return M->HasUpgradeAvailable(UpgradeKey);
	return false;
}

bool UBaseTransactionWidget::PayAndCheckComplete(FName UpgradeKey, AAlphaExilemetCharacter* Player)
{
	if (UUpgradeProgressionManager* M = GetProgressionManager())
		return M->PayTowardsUpgrade(UpgradeKey, Player);
	return false;
}

void UBaseTransactionWidget::NotifyUpgradeComplete(FName UpgradeKey, int32 LevelJustReached)
{
	if (UUpgradeProgressionManager* M = GetProgressionManager())
		M->AdvanceToNextLevelCost(UpgradeKey, LevelJustReached);
}
