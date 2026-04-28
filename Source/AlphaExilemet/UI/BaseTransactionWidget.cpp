#include "BaseTransactionWidget.h"
#include "AlphaExilemet/AlphaExilemetCharacter.h"
#include "AlphaExilemet/Core/AlphaExilemetGameInstance.h"
#include "AlphaExilemet/Core/UpgradeProgressionManager.h"
#include "Kismet/GameplayStatics.h"

// =========================================================================
// LEGACY API
// =========================================================================

bool UBaseTransactionWidget::CanAfford(FUpgradeCost CostInfo, AAlphaExilemetCharacter* Player)
{
	if (!Player) return false;

	if (Player->Currency < CostInfo.CurrencyCost) return false;

	for (const auto& MaterialPair : CostInfo.RequiredMaterials)
	{
		if (Player->GetTotalResourceAmount(MaterialPair.Key) < MaterialPair.Value) return false;
	}

	return true;
}

void UBaseTransactionWidget::DeductCost(FUpgradeCost CostInfo, AAlphaExilemetCharacter* Player)
{
	if (!Player) return;

	Player->Currency -= CostInfo.CurrencyCost;

	for (const auto& MaterialPair : CostInfo.RequiredMaterials)
	{
		Player->DeductResourceFromTools(MaterialPair.Key, MaterialPair.Value);
	}
}

// =========================================================================
// PARTIAL PAYMENT API
// =========================================================================

UUpgradeProgressionManager* UBaseTransactionWidget::GetProgressionManager() const
{
	UAlphaExilemetGameInstance* GI = Cast<UAlphaExilemetGameInstance>(
		UGameplayStatics::GetGameInstance(this)
	);
	return GI ? GI->ProgressionManager : nullptr;
}

FUpgradeCost UBaseTransactionWidget::GetCurrentCostForUpgrade(FName UpgradeKey) const
{
	if (UUpgradeProgressionManager* Manager = GetProgressionManager())
	{
		return Manager->GetCurrentCost(UpgradeKey);
	}
	return FUpgradeCost();
}

bool UBaseTransactionWidget::CanAffordFullUpgrade(FName UpgradeKey, AAlphaExilemetCharacter* Player) const
{
	if (UUpgradeProgressionManager* Manager = GetProgressionManager())
	{
		return Manager->CanAffordFull(UpgradeKey, Player);
	}
	return false;
}

bool UBaseTransactionWidget::IsUpgradeAvailable(FName UpgradeKey) const
{
	if (UUpgradeProgressionManager* Manager = GetProgressionManager())
	{
		return Manager->HasUpgradeAvailable(UpgradeKey);
	}
	return false;
}

bool UBaseTransactionWidget::PayAndCheckComplete(FName UpgradeKey, AAlphaExilemetCharacter* Player)
{
	if (UUpgradeProgressionManager* Manager = GetProgressionManager())
	{
		return Manager->PayTowardsUpgrade(UpgradeKey, Player);
	}
	return false;
}

void UBaseTransactionWidget::CompleteUpgrade(FName UpgradeKey, int32 LevelJustReached)
{
	if (UUpgradeProgressionManager* Manager = GetProgressionManager())
	{
		Manager->AdvanceToNextLevelCost(UpgradeKey, LevelJustReached);
	}
}
