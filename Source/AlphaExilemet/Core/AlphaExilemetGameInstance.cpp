#include "AlphaExilemetGameInstance.h"

#include "AlphaExilemet/Tools/ToolBase.h"
#include "AlphaExilemet/BaseCamp.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraComponent.h"

bool UAlphaExilemetGameInstance::DoesSaveExist(FString SlotName)
{
	return UGameplayStatics::DoesSaveGameExist(SlotName, 0);
}

void UAlphaExilemetGameInstance::CreateNewGame(FString SlotName)
{
	CurrentSaveSlot = SlotName;
	CurrentPhase    = EGamePhase::NewGame_Tutorial;

	LocalSaveRef = Cast<UAlphaExilemetSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UAlphaExilemetSaveGame::StaticClass()));

	if (LocalSaveRef)
	{
		LocalSaveRef->CurrentLevelName   = FName("Tutorial");
		LocalSaveRef->bHasValidTransform = false;
		UGameplayStatics::SaveGameToSlot(LocalSaveRef, CurrentSaveSlot, 0);
	}
}

void UAlphaExilemetGameInstance::SavePlayerData()
{
	PlayerRef = Cast<AAlphaExilemetCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!PlayerRef || !LocalSaveRef) return;

	LocalSaveRef->SavedHealth        = PlayerRef->Health;
	LocalSaveRef->SavedOxygen        = PlayerRef->Oxygen;
	LocalSaveRef->SavedCurrency      = PlayerRef->Currency;
	LocalSaveRef->SavedUpgradeLevels = PlayerRef->SystemUpgradeLevels;
	LocalSaveRef->PlayerLocation     = PlayerRef->GetActorTransform();

	if (PlayerRef->FirstPersonCameraComponent)
		LocalSaveRef->PlayerCamera = PlayerRef->FirstPersonCameraComponent->GetComponentTransform();

	PlayerRef->SaveToolDataToSaveObject(LocalSaveRef);
	LocalSaveRef->bHasValidTransform = true;
}

void UAlphaExilemetGameInstance::SetupPlayerData()
{
	PlayerRef = Cast<AAlphaExilemetCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!PlayerRef || !LocalSaveRef) return;

	if (LocalSaveRef->bHasValidTransform)
	{
		PlayerRef->SetActorTransform(
			LocalSaveRef->PlayerLocation, false, nullptr, ETeleportType::TeleportPhysics);
		if (APlayerController* PC = Cast<APlayerController>(PlayerRef->GetController()))
			PC->SetControlRotation(LocalSaveRef->PlayerCamera.Rotator());
	}

	PlayerRef->Health              = LocalSaveRef->SavedHealth;
	PlayerRef->Oxygen              = LocalSaveRef->SavedOxygen;
	PlayerRef->Currency            = LocalSaveRef->SavedCurrency;
	PlayerRef->SystemUpgradeLevels = LocalSaveRef->SavedUpgradeLevels;

	PlayerRef->RecalculateStats();
	PlayerRef->LoadToolDataFromSaveObject(LocalSaveRef);
}

void UAlphaExilemetGameInstance::SaveShipData()
{
	BaseRef = Cast<ABaseCamp>(UGameplayStatics::GetActorOfClass(this, ABaseCamp::StaticClass()));
	if (!BaseRef || !LocalSaveRef) return;
	LocalSaveRef->SavedShipRepairLevels = BaseRef->ShipRepairLevels;
}

void UAlphaExilemetGameInstance::SetupShipData()
{
	BaseRef = Cast<ABaseCamp>(UGameplayStatics::GetActorOfClass(this, ABaseCamp::StaticClass()));
	if (!BaseRef || !LocalSaveRef) return;
	BaseRef->ShipRepairLevels = LocalSaveRef->SavedShipRepairLevels;
	BaseRef->ApplyShipUpgrades();
}

// ─────────────────────────────────────────────────────────────────────────────
// SetupLoadedGame  (BUG 4 FIX)
// One call replaces the four individual nodes in GM LoadGamePlayer.
// ─────────────────────────────────────────────────────────────────────────────

void UAlphaExilemetGameInstance::SetupLoadedGame()
{
	// Order matters: player data first (teleport), then ship, then progression.
	SetupPlayerData();        // teleports to saved position if bHasValidTransform=true
	SetupShipData();          // restores ship repair levels + ApplyShipUpgrades
	InitProgressionManager(); // seeds upgrade costs from DataTables using current levels
	SetupProgressionData();   // overlays any saved partial payments on top

	UE_LOG(LogTemp, Log, TEXT("UAlphaExilemetGameInstance::SetupLoadedGame — complete."));
}

// ─────────────────────────────────────────────────────────────────────────────
// PROGRESSION
// ─────────────────────────────────────────────────────────────────────────────

void UAlphaExilemetGameInstance::InitProgressionManager()
{
	if (!ProgressionManager)
		ProgressionManager = NewObject<UUpgradeProgressionManager>(this);

	TMap<EPlayerStat, int32> CharacterLevels;
	TMap<FName, int32>       ToolStatLevels;
	TMap<EShipSystem, int32> ShipLevels;

	if (PlayerRef)
	{
		CharacterLevels = PlayerRef->SystemUpgradeLevels;
		for (AToolBase* Tool : PlayerRef->OwnedTools)
			if (Tool) ToolStatLevels.Append(Tool->ToolUpgradeLevels);
	}
	if (BaseRef)
		ShipLevels = BaseRef->ShipRepairLevels;

	ProgressionManager->InitializeFromDataTables(
		CharacterUpgradeTable, ToolUpgradeTable, ShipRepairTable,
		CharacterLevels, ToolStatLevels, ShipLevels);
}

void UAlphaExilemetGameInstance::SetupProgressionData()
{
	if (!ProgressionManager || !LocalSaveRef) return;
	ProgressionManager->LoadFromSaveObject(LocalSaveRef);
}

void UAlphaExilemetGameInstance::SaveProgressionData()
{
	if (!ProgressionManager || !LocalSaveRef) return;
	ProgressionManager->SaveToSaveObject(LocalSaveRef);
}
