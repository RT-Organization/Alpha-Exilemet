#include "AlphaExilemetGameInstance.h"
#include "AlphaExilemet/Core/AlphaStreamingSubsystem.h"

#include "AlphaExilemet/Tools/ToolBase.h"
#include "AlphaExilemet/BaseCamp.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraComponent.h"

// ─────────────────────────────────────────────────────────────────────────────
// HELPERS
// ─────────────────────────────────────────────────────────────────────────────

bool UAlphaExilemetGameInstance::IsPortalLevel(const FName& LevelName) const
{
	return !LevelName.IsNone()
		&& LevelName != FName("Tutorial")
		&& LevelName != FName("Main");
}

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
// SavePlayerData
//
// TUTORIAL GUARD: Never writes saves while in Tutorial. Quitting restarts it.
//
// PORTAL: Saves the portal level name as-is so loading brings the player
// back to the portal start. The challenge restarts from scratch (no mid-
// challenge state is saved — only position, health, tools, currency).
// ─────────────────────────────────────────────────────────────────────────────

void UAlphaExilemetGameInstance::SavePlayerData()
{
	if (!LocalSaveRef)
	{
		UE_LOG(LogTemp, Warning, TEXT("SavePlayerData: LocalSaveRef null — skipped."));
		return;
	}

	// ── TUTORIAL GUARD ───────────────────────────────────────────────────────
	if (LocalSaveRef->CurrentLevelName == FName("Tutorial"))
	{
		UE_LOG(LogTemp, Log,
			TEXT("SavePlayerData: Skipped — Tutorial saves are intentionally no-ops. "
			     "Tutorial restarts on next load."));
		return;
	}

	// ── ACTUAL SAVE ──────────────────────────────────────────────────────────
	// Portal levels are saved as-is. On load, LoadSaveAndStream will load
	// the portal level directly and restart the challenge.
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

	UE_LOG(LogTemp, Log,
		TEXT("SavePlayerData: Saved. Level='%s'"),
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
// LoadSaveAndStream
//
// Single C++ entry point called from WB_LoadMenu after the loading screen
// is already visible and LoadingScreenRef is set on the GM.
//
// ROUTING TABLE:
//   "Tutorial"   → NewGame_Tutorial → stream Tutorial, unload Main
//   portal name  → InPortal         → stream portal directly, unload Main
//                                     (challenge restarts, position in save ignored)
//   "Main"       → LoadedGame       → stream Main
//   none / empty → NewGame_Tutorial → Tutorial restart
// ─────────────────────────────────────────────────────────────────────────────

void UAlphaExilemetGameInstance::LoadSaveAndStream(FString SlotName)
{
	// ── 1. LOAD SAVE FILE ────────────────────────────────────────────────────
	if (!UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		UE_LOG(LogTemp, Error,
			TEXT("LoadSaveAndStream: No save found for slot '%s'."), *SlotName);
		return;
	}

	CurrentSaveSlot = SlotName;
	LocalSaveRef    = Cast<UAlphaExilemetSaveGame>(
		UGameplayStatics::LoadGameFromSlot(SlotName, 0));

	if (!LocalSaveRef)
	{
		UE_LOG(LogTemp, Error,
			TEXT("LoadSaveAndStream: Failed to cast save object for slot '%s'."), *SlotName);
		return;
	}

	// ── 2. READ SAVED LEVEL ──────────────────────────────────────────────────
	const FName SavedLevel = LocalSaveRef->CurrentLevelName;
	FName LevelToLoad;
	FName LevelToUnload;

	if (SavedLevel == FName("Tutorial") || SavedLevel.IsNone())
	{
		// Tutorial save (or corrupt) → restart Tutorial from scratch.
		LocalSaveRef->bHasValidTransform = false;
		CurrentPhase  = EGamePhase::NewGame_Tutorial;
		LevelToLoad   = FName("Tutorial");
		LevelToUnload = FName("Main");

		UE_LOG(LogTemp, Log,
			TEXT("LoadSaveAndStream: Tutorial/empty save → restarting Tutorial."));
	}
	else if (IsPortalLevel(SavedLevel))
	{
		// Portal save → load the portal directly.
		// GM switch will hit InPortal → TeleportPlayerToPortalStart → restart challenge.
		// bHasValidTransform is intentionally NOT reset: if the portal has a
		// PlayerStart, TeleportPlayerToPortalStart() ignores it anyway.
		CurrentPhase  = EGamePhase::InPortal;
		LevelToLoad   = SavedLevel;
		LevelToUnload = FName("Main");

		// Tell the streaming subsystem which portal is active so
		// TeleportPlayerToPortalStart can find it.
		if (UAlphaStreamingSubsystem* SS = GetSubsystem<UAlphaStreamingSubsystem>())
		{
			SS->SetActivePortalName(SavedLevel);
		}

		UE_LOG(LogTemp, Log,
			TEXT("LoadSaveAndStream: Portal save '%s' → loading portal, restarting challenge."),
			*SavedLevel.ToString());
	}
	else
	{
		// "Main" or any named main-gameplay level → normal loaded game.
		CurrentPhase  = EGamePhase::LoadedGame;
		LevelToLoad   = FName("Main");
		LevelToUnload = NAME_None;

		UE_LOG(LogTemp, Log,
			TEXT("LoadSaveAndStream: Main save → LoadedGame."));
	}

	// ── 3. STREAM ────────────────────────────────────────────────────────────
	UAlphaStreamingSubsystem* SS = GetSubsystem<UAlphaStreamingSubsystem>();
	if (!SS)
	{
		UE_LOG(LogTemp, Error,
			TEXT("LoadSaveAndStream: AlphaStreamingSubsystem not found."));
		return;
	}

	SS->StreamLevel(LevelToLoad, LevelToUnload);
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
