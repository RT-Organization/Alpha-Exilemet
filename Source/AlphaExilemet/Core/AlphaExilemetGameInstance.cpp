#include "AlphaExilemetGameInstance.h"
#include "AlphaExilemet/Core/LevelStreamingManager.h"

#include "AlphaExilemet/Tools/ToolBase.h"
#include "AlphaExilemet/BaseCamp.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraComponent.h"

// ─────────────────────────────────────────────────────────────────────────────
// NOTE: All level streaming logic has moved to ULevelStreamingManager.
// This class handles only: save/load data, ship data, progression.
// ─────────────────────────────────────────────────────────────────────────────

// ─────────────────────────────────────────────────────────────────────────────
// DoesSaveExist / CreateNewGame
// ─────────────────────────────────────────────────────────────────────────────

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

// ─────────────────────────────────────────────────────────────────────────────
// LoadSavedGame — delegates entirely to LevelStreamingManager
// ─────────────────────────────────────────────────────────────────────────────

void UAlphaExilemetGameInstance::LoadSavedGame(FString SlotName)
{
	if (ULevelStreamingManager* M = GetSubsystem<ULevelStreamingManager>())
		M->LoadSavedGame(SlotName);
	else
		UE_LOG(LogTemp, Error, TEXT("UAlphaExilemetGameInstance::LoadSavedGame — LevelStreamingManager not found."));
}

// ─────────────────────────────────────────────────────────────────────────────
// DEPRECATED — kept for any BP that still calls the old name
// ─────────────────────────────────────────────────────────────────────────────

void UAlphaExilemetGameInstance::LoadSaveAndStream(FString SlotName)
{
	// Forward to the new function.
	LoadSavedGame(SlotName);
}

// ─────────────────────────────────────────────────────────────────────────────
// SavePlayerData
// ─────────────────────────────────────────────────────────────────────────────

void UAlphaExilemetGameInstance::SavePlayerData()
{
	if (!LocalSaveRef)
	{
		UE_LOG(LogTemp, Warning, TEXT("SavePlayerData: LocalSaveRef null — skipped."));
		return;
	}

	// Tutorial guard.
	if (LocalSaveRef->CurrentLevelName == FName("Tutorial"))
	{
		UE_LOG(LogTemp, Log, TEXT("SavePlayerData: Skipped — Tutorial."));
		return;
	}

	PlayerRef = Cast<AAlphaExilemetCharacter>(
		UGameplayStatics::GetPlayerCharacter(this, 0));

	if (!PlayerRef)
	{
		UE_LOG(LogTemp, Warning, TEXT("SavePlayerData: PlayerRef not found."));
		return;
	}

	LocalSaveRef->SavedHealth        = PlayerRef->Health;
	LocalSaveRef->SavedOxygen        = PlayerRef->Oxygen;
	LocalSaveRef->SavedCurrency      = PlayerRef->Currency;
	LocalSaveRef->SavedUpgradeLevels = PlayerRef->SystemUpgradeLevels;
	LocalSaveRef->PlayerLocation     = PlayerRef->GetActorTransform();

	if (PlayerRef->FirstPersonCameraComponent)
		LocalSaveRef->PlayerCamera =
			PlayerRef->FirstPersonCameraComponent->GetComponentTransform();

	PlayerRef->SaveToolDataToSaveObject(LocalSaveRef);
	LocalSaveRef->bHasValidTransform = true;

	UGameplayStatics::SaveGameToSlot(LocalSaveRef, CurrentSaveSlot, 0);

	UE_LOG(LogTemp, Log, TEXT("SavePlayerData: Saved. Level='%s'"),
		*LocalSaveRef->CurrentLevelName.ToString());
}

// ─────────────────────────────────────────────────────────────────────────────
// SetupPlayerData
// ─────────────────────────────────────────────────────────────────────────────

void UAlphaExilemetGameInstance::SetupPlayerData()
{
	PlayerRef = Cast<AAlphaExilemetCharacter>(
		UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!PlayerRef || !LocalSaveRef) return;

	if (LocalSaveRef->bHasValidTransform)
	{
		PlayerRef->SetActorTransform(
			LocalSaveRef->PlayerLocation,
			false, nullptr, ETeleportType::TeleportPhysics);

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

// ─────────────────────────────────────────────────────────────────────────────
// SaveShipData / SetupShipData
// ─────────────────────────────────────────────────────────────────────────────

void UAlphaExilemetGameInstance::SaveShipData()
{
	BaseRef = Cast<ABaseCamp>(
		UGameplayStatics::GetActorOfClass(this, ABaseCamp::StaticClass()));
	if (!BaseRef || !LocalSaveRef) return;
	LocalSaveRef->SavedShipRepairLevels = BaseRef->ShipRepairLevels;
}

void UAlphaExilemetGameInstance::SetupShipData()
{
	BaseRef = Cast<ABaseCamp>(
		UGameplayStatics::GetActorOfClass(this, ABaseCamp::StaticClass()));
	if (!BaseRef || !LocalSaveRef) return;
	BaseRef->ShipRepairLevels = LocalSaveRef->SavedShipRepairLevels;
	BaseRef->ApplyShipUpgrades();
}

// ─────────────────────────────────────────────────────────────────────────────
// SetupLoadedGame
// ─────────────────────────────────────────────────────────────────────────────

void UAlphaExilemetGameInstance::SetupLoadedGame()
{
	SetupPlayerData();
	SetupShipData();
	InitProgressionManager();
	SetupProgressionData();
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
