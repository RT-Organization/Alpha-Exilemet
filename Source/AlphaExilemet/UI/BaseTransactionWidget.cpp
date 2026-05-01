#include "BaseTransactionWidget.h"
#include "AlphaExilemet/AlphaExilemetCharacter.h"
#include "AlphaExilemet/Core/AlphaExilemetGameInstance.h"
#include "AlphaExilemet/Core/UpgradeProgressionManager.h"
#include "Kismet/GameplayStatics.h"

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
// LEGACY API (Sell Terminal)
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
// UPGRADE API (all three row widgets)
// ─────────────────────────────────────────────────────────────────────────────

FUpgradeCost UBaseTransactionWidget::GetRemainingCost(FName UpgradeKey) const
{
	if (UUpgradeProgressionManager* M = GetProgressionManager())
		return M->GetCurrentCost(UpgradeKey);
	return FUpgradeCost();
}

bool UBaseTransactionWidget::CanPayAnything(FName UpgradeKey, AAlphaExilemetCharacter* Player) const
{
	if (UUpgradeProgressionManager* M = GetProgressionManager())
		return M->CanPayAnything(UpgradeKey, Player);
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
