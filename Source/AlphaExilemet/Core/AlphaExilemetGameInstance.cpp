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
	// Any level name that is not Tutorial and not Main is treated as a portal.
	// This means new portal levels work automatically with no code change needed.
	return !LevelName.IsNone()
		&& LevelName != FName("Tutorial")
		&& LevelName != FName("Main");
}

// ─────────────────────────────────────────────────────────────────────────────
// DoesSaveExist
// ─────────────────────────────────────────────────────────────────────────────

bool UAlphaExilemetGameInstance::DoesSaveExist(FString SlotName)
{
	return UGameplayStatics::DoesSaveGameExist(SlotName, 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// CreateNewGame
// ─────────────────────────────────────────────────────────────────────────────

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
// TUTORIAL GUARD: If current level is Tutorial, do nothing.
//   Tutorial progress is never written to disk.
//   If the player quits in Tutorial, next load restarts from scratch.
//
// PORTAL GUARD: If current level is a portal, write CurrentLevelName = "Main".
//   Portal challenges always restart. On load the player resumes in Main,
//   not inside a half-finished portal.
// ─────────────────────────────────────────────────────────────────────────────

void UAlphaExilemetGameInstance::SavePlayerData()
{
	if (!LocalSaveRef)
	{
		UE_LOG(LogTemp, Warning, TEXT("SavePlayerData: LocalSaveRef is null — skipped."));
		return;
	}

	// ── TUTORIAL GUARD ───────────────────────────────────────────────────────
	if (LocalSaveRef->CurrentLevelName == FName("Tutorial"))
	{
		UE_LOG(LogTemp, Log,
			TEXT("SavePlayerData: Skipped — Tutorial level saves are intentionally ignored. "
			     "Tutorial will restart from the beginning on next load."));
		return;
	}

	// ── PORTAL GUARD ─────────────────────────────────────────────────────────
	// If the save was written while inside a portal, redirect to Main so the
	// load menu always brings the player back to the Main level, not a portal.
	if (IsPortalLevel(LocalSaveRef->CurrentLevelName))
	{
		UE_LOG(LogTemp, Log,
			TEXT("SavePlayerData: Portal level '%s' — overriding CurrentLevelName to 'Main'. "
			     "Portal challenge will restart on next load."),
			*LocalSaveRef->CurrentLevelName.ToString());

		LocalSaveRef->CurrentLevelName = FName("Main");
	}

	// ── ACTUAL SAVE ──────────────────────────────────────────────────────────
	PlayerRef = Cast<AAlphaExilemetCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!PlayerRef)
	{
		UE_LOG(LogTemp, Warning, TEXT("SavePlayerData: PlayerRef not found — data not written."));
		return;
	}

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

// ─────────────────────────────────────────────────────────────────────────────
// SetupPlayerData
// ─────────────────────────────────────────────────────────────────────────────

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

// ─────────────────────────────────────────────────────────────────────────────
// SaveShipData / SetupShipData
// ─────────────────────────────────────────────────────────────────────────────

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
// Single entry point for the Load Menu. Replaces the entire WB_LoadMenu
// "Load Slot Game" BP chain from save-file loading to streaming.
//
// The loading screen MUST already be visible before calling this.
//
// WB_LoadMenu Load Slot — new BP chain (three nodes only):
//   [Load Slot  custom event]
//     → [Get Game Instance → Cast To AlphaExilemetGameInstance]
//     → [LoadSaveAndStream  (Slot Name = Slot Save pin)]
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

	LocalSaveRef = Cast<UAlphaExilemetSaveGame>(
		UGameplayStatics::LoadGameFromSlot(SlotName, 0));

	if (!LocalSaveRef)
	{
		UE_LOG(LogTemp, Error,
			TEXT("LoadSaveAndStream: Failed to load save from slot '%s'."), *SlotName);
		return;
	}

	// ── 2. DETERMINE LEVEL AND PHASE ────────────────────────────────────────
	const FName SavedLevel = LocalSaveRef->CurrentLevelName;
	FName LevelToLoad;
	FName LevelToUnload;

	if (SavedLevel == FName("Tutorial") || SavedLevel.IsNone())
	{
		// Tutorial save → restart Tutorial from the very beginning.
		// Reset bHasValidTransform so SetupPlayerData doesn't teleport to zero.
		LocalSaveRef->bHasValidTransform = false;
		CurrentPhase = EGamePhase::NewGame_Tutorial;
		LevelToLoad   = FName("Tutorial");
		LevelToUnload = FName("Main");

		UE_LOG(LogTemp, Log,
			TEXT("LoadSaveAndStream: Tutorial save detected — restarting Tutorial."));
	}
	else if (IsPortalLevel(SavedLevel))
	{
		// Portal save → load Main level only.
		// The portal challenge restarts: GM will NOT call EnterPortal automatically.
		// The player resumes in Main at their last Main-level position.
		// Because SavePlayerData() already wrote CurrentLevelName = "Main" for portal
		// saves, this branch mostly handles old saves or manual edits.
		LocalSaveRef->CurrentLevelName = FName("Main");
		CurrentPhase = EGamePhase::LoadedGame;
		LevelToLoad   = FName("Main");
		LevelToUnload = NAME_None;

		UE_LOG(LogTemp, Log,
			TEXT("LoadSaveAndStream: Portal save '%s' — loading Main, portal challenge restarted."),
			*SavedLevel.ToString());
	}
	else // "Main" or any future named main-gameplay level
	{
		CurrentPhase = EGamePhase::LoadedGame;
		LevelToLoad   = FName("Main");
		LevelToUnload = NAME_None;

		UE_LOG(LogTemp, Log,
			TEXT("LoadSaveAndStream: Loading Main level (phase = LoadedGame)."));
	}

	// ── 3. STREAM ────────────────────────────────────────────────────────────
	// AlphaStreamingSubsystem::OnStreamLevelLoaded fires when the level is ready.
	// GM_SimulatorGamemode's OnAnyLevelStreamComplete handles the rest via Switch on Phase.
	UAlphaStreamingSubsystem* SS = GetSubsystem<UAlphaStreamingSubsystem>();
	if (!SS)
	{
		UE_LOG(LogTemp, Error, TEXT("LoadSaveAndStream: AlphaStreamingSubsystem not found."));
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