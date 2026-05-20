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
		&& LevelName != FName("Main")
		&& LevelName != FName("MainMenu");
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
// ─────────────────────────────────────────────────────────────────────────────

void UAlphaExilemetGameInstance::SavePlayerData()
{
	if (!LocalSaveRef)
	{
		UE_LOG(LogTemp, Warning, TEXT("SavePlayerData: LocalSaveRef null — skipped."));
		return;
	}

	// Tutorial guard — Tutorial saves are no-ops.
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
// LoadSaveAndStream
//
// IMPORTANT — L_Persistent problem:
//
// L_Persistent::BeginPlay always calls LoadStreamLevel("MainMenu"). By the
// time WB_LoadMenu calls LoadSaveAndStream, MainMenu is already loaded.
// CurrentActiveLevel may still be NAME_None if InitializeInstance hasn't
// finished yet (it depends on timing).
//
// Fix: we ALWAYS unload MainMenu explicitly when loading a game, regardless
// of what CurrentActiveLevel says. We know MainMenu is loaded because
// L_Persistent loads it unconditionally on startup.
//
// ROUTING TABLE:
//   "Tutorial"   → NewGame_Tutorial → stream Tutorial, unload MainMenu
//   portal name  → InPortal         → stream portal, unload MainMenu
//   "Main"       → LoadedGame       → stream Main, unload MainMenu
//   none/empty   → NewGame_Tutorial → Tutorial restart, unload MainMenu
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
			TEXT("LoadSaveAndStream: Failed to cast save for slot '%s'."), *SlotName);
		return;
	}

	// ── 2. GET SUBSYSTEM ─────────────────────────────────────────────────────
	UAlphaStreamingSubsystem* SS = GetSubsystem<UAlphaStreamingSubsystem>();
	if (!SS)
	{
		UE_LOG(LogTemp, Error, TEXT("LoadSaveAndStream: AlphaStreamingSubsystem not found."));
		return;
	}

	// ── 3. DETERMINE WHAT TO LOAD ────────────────────────────────────────────
	const FName SavedLevel = LocalSaveRef->CurrentLevelName;
	FName LevelToLoad;

	if (SavedLevel == FName("Tutorial") || SavedLevel.IsNone())
	{
		LocalSaveRef->bHasValidTransform = false;
		CurrentPhase = EGamePhase::NewGame_Tutorial;
		LevelToLoad  = FName("Tutorial");

		// Tell the subsystem the active level is now Tutorial.
		SS->SetCurrentActiveLevel(FName("Tutorial"));

		UE_LOG(LogTemp, Log,
			TEXT("LoadSaveAndStream: Tutorial/empty save → restarting Tutorial."));
	}
	else if (IsPortalLevel(SavedLevel))
	{
		CurrentPhase = EGamePhase::InPortal;
		LevelToLoad  = SavedLevel;

		SS->SetActivePortalName(SavedLevel);
		SS->SetCurrentActiveLevel(SavedLevel);

		UE_LOG(LogTemp, Log,
			TEXT("LoadSaveAndStream: Portal save '%s' → loading portal."),
			*SavedLevel.ToString());
	}
	else
	{
		// "Main" or any named main-gameplay level.
		CurrentPhase = EGamePhase::LoadedGame;
		LevelToLoad  = FName("Main");

		SS->SetCurrentActiveLevel(FName("Main"));

		UE_LOG(LogTemp, Log, TEXT("LoadSaveAndStream: Main save → LoadedGame."));
	}

	// ── 4. STREAM ────────────────────────────────────────────────────────────
	// We ALWAYS unload MainMenu here because L_Persistent::BeginPlay always
	// loads it unconditionally. Even if CurrentActiveLevel was NAME_None,
	// MainMenu is guaranteed to be in memory at this point.
	//
	// NOTE: We call StreamLevel directly (not SS->StreamLevel through the
	// Tutorial path) because bTutorialTransition must NOT be set here —
	// this is a load-from-save path, not a Tutorial completion path.
	SS->StreamLevel(LevelToLoad, FName("MainMenu"));

	UE_LOG(LogTemp, Log,
		TEXT("LoadSaveAndStream: loading '%s', unloading 'MainMenu'."),
		*LevelToLoad.ToString());
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