#include "LevelStreamingManager.h"

#include "Kismet/GameplayStatics.h"
#include "Engine/LevelStreaming.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"
#include "TimerManager.h"

#include "AlphaExilemetGameInstance.h"
#include "AlphaExilemet/AlphaExilemetCharacter.h"
#include "AlphaExilemet/Tools/ToolBase.h"
#include "AlphaExilemet/Core/AlphaExilemetSaveGame.h"

// ─────────────────────────────────────────────────────────────────────────────
// UTILITY — Level Name Conversion
// ─────────────────────────────────────────────────────────────────────────────

FName ULevelStreamingManager::LevelToName(EGameLevel Level)
{
	switch (Level)
	{
		case EGameLevel::MainMenu:      return FName("MainMenu");
		case EGameLevel::Tutorial:      return FName("Tutorial");
		case EGameLevel::Main:          return FName("Main");
		case EGameLevel::Portal_Desert: return FName("Portal_Desert");
		case EGameLevel::Portal_Canyon: return FName("Portal_Canyon");
		case EGameLevel::Portal_Ice:    return FName("Portal_Ice");
		case EGameLevel::Portal_Lava:   return FName("Portal_Lava");
		case EGameLevel::Portal_Swamp:  return FName("Portal_Swamp");
		default:                        return NAME_None;
	}
}

EGameLevel ULevelStreamingManager::NameToLevel(FName Name)
{
	if (Name == "MainMenu")      return EGameLevel::MainMenu;
	if (Name == "Tutorial")      return EGameLevel::Tutorial;
	if (Name == "Main")          return EGameLevel::Main;
	if (Name == "Portal_Desert") return EGameLevel::Portal_Desert;
	if (Name == "Portal_Canyon") return EGameLevel::Portal_Canyon;
	if (Name == "Portal_Ice")    return EGameLevel::Portal_Ice;
	if (Name == "Portal_Lava")   return EGameLevel::Portal_Lava;
	if (Name == "Portal_Swamp")  return EGameLevel::Portal_Swamp;
	return EGameLevel::None;
}

bool ULevelStreamingManager::IsPortalLevel(EGameLevel Level)
{
	return Level == EGameLevel::Portal_Desert
		|| Level == EGameLevel::Portal_Canyon
		|| Level == EGameLevel::Portal_Ice
		|| Level == EGameLevel::Portal_Lava
		|| Level == EGameLevel::Portal_Swamp;
}

// ─────────────────────────────────────────────────────────────────────────────
// SETUP
// ─────────────────────────────────────────────────────────────────────────────

void ULevelStreamingManager::InitializeAtMainMenu(TSubclassOf<UUserWidget> InLoadingScreenClass)
{
	if (InLoadingScreenClass)
		LoadingScreenClass = InLoadingScreenClass;

	// L_Persistent always loads MainMenu at startup — we just register that fact.
	CurrentLevel         = EGameLevel::MainMenu;
	bTransitionInProgress = false;

	UE_LOG(LogTemp, Log, TEXT("LevelStreamingManager: initialized at MainMenu."));
}

// ─────────────────────────────────────────────────────────────────────────────
// PUBLIC TRANSITION FUNCTIONS
// ─────────────────────────────────────────────────────────────────────────────

// ── START NEW GAME ────────────────────────────────────────────────────────────

void ULevelStreamingManager::StartNewGame(const FString& SlotName, bool bSeamless)
{
	if (bTransitionInProgress)
	{
		UE_LOG(LogTemp, Warning, TEXT("LevelStreamingManager::StartNewGame — transition in progress."));
		return;
	}

	// Create a fresh save for the new game.
	UAlphaExilemetGameInstance* GI = Cast<UAlphaExilemetGameInstance>(GetGameInstance());
	if (!GI)
	{
		UE_LOG(LogTemp, Error, TEXT("LevelStreamingManager::StartNewGame — GameInstance not found."));
		return;
	}

	// CreateNewGame sets CurrentPhase = NewGame_Tutorial and writes the save file.
	GI->CreateNewGame(SlotName);

	UE_LOG(LogTemp, Log,
		TEXT("LevelStreamingManager::StartNewGame — slot='%s', seamless=%s."),
		*SlotName, bSeamless ? TEXT("true") : TEXT("false"));

	// Always unload MainMenu (L_Persistent guarantees it is loaded).
	BeginTransition(EGameLevel::Tutorial, EGameLevel::MainMenu, bSeamless);
}

// ── LOAD SAVED GAME ──────────────────────────────────────────────────────────

void ULevelStreamingManager::LoadSavedGame(const FString& SlotName)
{
	if (bTransitionInProgress)
	{
		UE_LOG(LogTemp, Warning, TEXT("LevelStreamingManager::LoadSavedGame — transition already in progress."));
		return;
	}

	// Load the save file.
	if (!UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		UE_LOG(LogTemp, Error, TEXT("LevelStreamingManager::LoadSavedGame — no save in slot '%s'."), *SlotName);
		return;
	}

	UAlphaExilemetGameInstance* GI = Cast<UAlphaExilemetGameInstance>(GetGameInstance());
	if (!GI) return;

	GI->CurrentSaveSlot = SlotName;
	GI->LocalSaveRef    = Cast<UAlphaExilemetSaveGame>(
		UGameplayStatics::LoadGameFromSlot(SlotName, 0));

	if (!GI->LocalSaveRef)
	{
		UE_LOG(LogTemp, Error, TEXT("LevelStreamingManager::LoadSavedGame — failed to cast save."));
		return;
	}

	// Determine target level from save.
	const FName SavedLevelName = GI->LocalSaveRef->CurrentLevelName;
	EGameLevel  TargetLevel    = NameToLevel(SavedLevelName);

	if (TargetLevel == EGameLevel::None || TargetLevel == EGameLevel::Tutorial)
	{
		// Tutorial save or corrupt → restart Tutorial fresh.
		TargetLevel = EGameLevel::Tutorial;
		GI->LocalSaveRef->bHasValidTransform = false;
		GI->CurrentPhase = EGamePhase::NewGame_Tutorial;
	}
	else if (IsPortalLevel(TargetLevel))
	{
		GI->CurrentPhase = EGamePhase::InPortal;
		PortalReturnFrom = EGameLevel::Main;

		// Store active portal so TeleportPlayerToPortalStart can find it.
		// We set this before the transition so it's ready when OnLevelTransitionComplete fires.
	}
	else
	{
		// Main or other named level.
		GI->CurrentPhase = EGamePhase::LoadedGame;
		TargetLevel      = EGameLevel::Main;
	}

	UE_LOG(LogTemp, Log, TEXT("LevelStreamingManager::LoadSavedGame — loading '%s' from slot '%s'."),
		*LevelToName(TargetLevel).ToString(), *SlotName);

	// Always unload MainMenu (L_Persistent guarantees it is loaded).
	BeginTransition(TargetLevel, EGameLevel::MainMenu);
}

// ── RETURN TO MAIN MENU ──────────────────────────────────────────────────────

void ULevelStreamingManager::ReturnToMainMenu()
{
	if (bTransitionInProgress)
	{
		UE_LOG(LogTemp, Warning, TEXT("LevelStreamingManager::ReturnToMainMenu — transition in progress."));
		return;
	}

	EGameLevel FromLevel = CurrentLevel;

	UE_LOG(LogTemp, Log, TEXT("LevelStreamingManager::ReturnToMainMenu — from '%s'."),
		*LevelToName(FromLevel).ToString());

	BeginTransition(EGameLevel::MainMenu, FromLevel);
}

// ── TUTORIAL → MAIN ──────────────────────────────────────────────────────────

void ULevelStreamingManager::CompleteTutorialAndLoadMain()
{
	if (bTransitionInProgress) return;

	// 1. Clear all player tools — authoritative clear.
	if (AAlphaExilemetCharacter* Player = GetLocalPlayer())
	{
		for (AToolBase* Tool : Player->OwnedTools)
			if (Tool) Tool->Destroy();

		Player->OwnedTools.Empty();
		Player->CurrentTool      = nullptr;
		Player->ActiveToolIndex  = -1;
		Player->PendingToolIndex = -1;
		Player->OnInventoryUpdated.Broadcast();
		Player->OnToolWielded.Broadcast(-1);
	}

	// 2. Save with CurrentLevelName = Main so load-back goes to Main.
	if (UAlphaExilemetGameInstance* GI = Cast<UAlphaExilemetGameInstance>(GetGameInstance()))
	{
		GI->CurrentPhase = EGamePhase::Main;
		if (GI->LocalSaveRef)
			GI->LocalSaveRef->CurrentLevelName = FName("Main");
		GI->SavePlayerData();
	}

	UE_LOG(LogTemp, Log, TEXT("LevelStreamingManager::CompleteTutorialAndLoadMain."));

	BeginTransition(EGameLevel::Main, EGameLevel::Tutorial);
}

// ── PORTAL ENTER ─────────────────────────────────────────────────────────────

void ULevelStreamingManager::EnterPortal(EGameLevel PortalLevel, FTransform PlayerReturnTransform)
{
	if (bTransitionInProgress) return;
	if (!IsPortalLevel(PortalLevel))
	{
		UE_LOG(LogTemp, Warning, TEXT("LevelStreamingManager::EnterPortal — '%s' is not a portal level."),
			*LevelToName(PortalLevel).ToString());
		return;
	}

	// Save return state.
	PortalReturnFrom      = CurrentLevel;
	PortalReturnTransform = PlayerReturnTransform;

	if (UAlphaExilemetGameInstance* GI = Cast<UAlphaExilemetGameInstance>(GetGameInstance()))
	{
		GI->CurrentPhase = EGamePhase::InPortal;
		if (GI->LocalSaveRef)
		{
			GI->LocalSaveRef->PrePortalTransform = PlayerReturnTransform;
			GI->LocalSaveRef->CurrentLevelName   = LevelToName(PortalLevel);
		}
		GI->SavePlayerData();
	}

	// Disable survival during challenge.
	if (AAlphaExilemetCharacter* Player = GetLocalPlayer())
		Player->bIsSurvivalActive = false;

	UE_LOG(LogTemp, Log, TEXT("LevelStreamingManager::EnterPortal — loading '%s'."),
		*LevelToName(PortalLevel).ToString());

	BeginTransition(PortalLevel, PortalReturnFrom);
}

// ── PORTAL EXIT ──────────────────────────────────────────────────────────────

void ULevelStreamingManager::ExitPortal()
{
	if (bTransitionInProgress) return;

	EGameLevel FromPortal = CurrentLevel;

	// Restore return transform.
	if (UAlphaExilemetGameInstance* GI = Cast<UAlphaExilemetGameInstance>(GetGameInstance()))
	{
		GI->CurrentPhase = EGamePhase::Main;
		if (GI->LocalSaveRef)
		{
			PortalReturnTransform              = GI->LocalSaveRef->PrePortalTransform;
			GI->LocalSaveRef->CurrentLevelName = FName("Main");
		}
		GI->SavePlayerData();
	}

	// Teleport player to return position BEFORE streaming starts.
	if (AAlphaExilemetCharacter* Player = GetLocalPlayer())
	{
		Player->SetActorTransform(PortalReturnTransform, false, nullptr, ETeleportType::TeleportPhysics);
		if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
			PC->SetControlRotation(PortalReturnTransform.GetRotation().Rotator());
		Player->bIsSurvivalActive = true;
	}

	UE_LOG(LogTemp, Log, TEXT("LevelStreamingManager::ExitPortal — from '%s'."),
		*LevelToName(FromPortal).ToString());

	BeginTransition(EGameLevel::Main, FromPortal);
}

void ULevelStreamingManager::CompletePortalChallenge()
{
	// Signal challenge code that the portal is done — then call ExitPortal.
	OnPortalExitStarted.Broadcast(CurrentLevel, EGameLevel::Main);
	ExitPortal();
}

void ULevelStreamingManager::Debug_ForceExitPortal()
{
	UE_LOG(LogTemp, Warning, TEXT("LevelStreamingManager: Debug_ForceExitPortal called."));
	ExitPortal();
}

// ─────────────────────────────────────────────────────────────────────────────
// CORE TRANSITION PIPELINE
//
// This is the heart of the system. Every transition goes through here.
// The pipeline:
//   BeginTransition → ShowLoadingScreen → start MinTime timer → UnloadOld
//   → LoadNew → OnLevelLoaded sets bLevelLoaded
//                OnMinTimeElapsed sets bMinTimeElapsed
//   → OnBothConditionsMet → HideLoadingScreen → OnLevelTransitionComplete
// ─────────────────────────────────────────────────────────────────────────────

void ULevelStreamingManager::BeginTransition(EGameLevel NewLevel, EGameLevel OldLevel, bool bSeamless)
{
	bTransitionInProgress  = true;
	bLevelLoaded           = false;
	bMinTimeElapsed        = false;
	bSeamlessTransition    = bSeamless;
	PendingLevel           = NewLevel;

	UE_LOG(LogTemp, Log, TEXT("LevelStreamingManager::BeginTransition: '%s' → '%s'%s."),
		*LevelToName(OldLevel).ToString(), *LevelToName(NewLevel).ToString(),
		bSeamless ? TEXT(" [SEAMLESS]") : TEXT(""));

	// 1. Notify GM to clean up (destroy player, clear refs).
	OnLevelTransitionStarted.Broadcast(OldLevel, NewLevel);

	// 2. Loading screen — skip entirely in seamless mode.
	//    In seamless mode the MainMenu animation is the "loading screen" —
	//    the level loads silently behind it and the camera cut happens when
	//    the Tutorial's TutorialDirector takes over.
	if (!bSeamless)
	{
		ShowLoadingScreen();

		// 3. Start minimum display timer.
		GetWorld()->GetTimerManager().SetTimer(
			MinTimeHandle,
			this,
			&ULevelStreamingManager::OnMinTimeElapsed,
			MinLoadingScreenTime,
			false);
	}
	else
	{
		// Seamless: min time is considered already elapsed.
		// OnBothConditionsMet will fire as soon as the level loads.
		bMinTimeElapsed = true;
	}

	UWorld* World = GetWorld();
	if (!World) return;

	// 4. Unload old level (silent).
	FName OldName = LevelToName(OldLevel);
	if (!OldName.IsNone())
	{
		FLatentActionInfo UnloadInfo;
		UnloadInfo.CallbackTarget    = this;
		UnloadInfo.ExecutionFunction = FName("OnLevelUnloaded");
		UnloadInfo.Linkage           = 0;
		UnloadInfo.UUID              = ++UnloadLatentUUID;
		UGameplayStatics::UnloadStreamLevel(World, OldName, UnloadInfo, false);
	}

	// 5. Load new level → OnLevelLoaded fires when complete.
	FName NewName = LevelToName(NewLevel);
	if (!NewName.IsNone())
	{
		FLatentActionInfo LoadInfo;
		LoadInfo.CallbackTarget    = this;
		LoadInfo.ExecutionFunction = FName("OnLevelLoaded");
		LoadInfo.Linkage           = 0;
		LoadInfo.UUID              = ++LoadLatentUUID;
		UGameplayStatics::LoadStreamLevel(World, NewName, true, true, LoadInfo);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("LevelStreamingManager::BeginTransition — NewLevel has no name!"));
		bTransitionInProgress = false;
	}

	// Update tracking immediately.
	CurrentLevel = NewLevel;
}

// ─────────────────────────────────────────────────────────────────────────────
// CALLBACKS
// ─────────────────────────────────────────────────────────────────────────────

void ULevelStreamingManager::OnLevelLoaded()
{
	UE_LOG(LogTemp, Log, TEXT("LevelStreamingManager: level '%s' loaded."),
		*LevelToName(CurrentLevel).ToString());

	bLevelLoaded = true;
	OnBothConditionsMet();
}

void ULevelStreamingManager::OnLevelUnloaded()
{
	// Intentionally silent. Unloads never drive game logic.
	UE_LOG(LogTemp, Verbose, TEXT("LevelStreamingManager: level unloaded (silent)."));
}

void ULevelStreamingManager::OnMinTimeElapsed()
{
	UE_LOG(LogTemp, Log, TEXT("LevelStreamingManager: minimum loading time elapsed."));
	bMinTimeElapsed = true;
	OnBothConditionsMet();
}

void ULevelStreamingManager::OnBothConditionsMet()
{
	// Both conditions must be true: level loaded AND min time elapsed.
	if (!bLevelLoaded || !bMinTimeElapsed) return;

	UE_LOG(LogTemp, Log,
		TEXT("LevelStreamingManager: both conditions met for '%s'%s."),
		*LevelToName(CurrentLevel).ToString(),
		bSeamlessTransition ? TEXT(" [SEAMLESS — no loading screen to hide]") : TEXT(""));

	// Only hide loading screen if we showed one.
	if (!bSeamlessTransition)
	{
		HideLoadingScreen();
	}

	bTransitionInProgress  = false;
	bSeamlessTransition    = false;

	// Fire the single delegate that drives all game logic.
	OnLevelTransitionComplete.Broadcast(CurrentLevel);
}

// ─────────────────────────────────────────────────────────────────────────────
// LOADING SCREEN
// ─────────────────────────────────────────────────────────────────────────────

void ULevelStreamingManager::ShowLoadingScreen()
{
	if (!LoadingScreenClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("LevelStreamingManager::ShowLoadingScreen — LoadingScreenClass not set!"));
		return;
	}

	// Destroy any existing loading screen first.
	if (ActiveLoadingScreen)
	{
		ActiveLoadingScreen->RemoveFromParent();
		ActiveLoadingScreen = nullptr;
	}

	// CreateWidget requires a UWorld* or APlayerController* as the outer.
	// Always prefer PlayerController so the widget is owned by the local player.
	// Fall back to GetWorld() if no controller exists yet (e.g. MainMenu state).
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("LevelStreamingManager::ShowLoadingScreen — no World!"));
		return;
	}

	APlayerController* PC = World->GetFirstPlayerController();
	if (PC)
	{
		ActiveLoadingScreen = CreateWidget<UUserWidget>(PC, LoadingScreenClass);
	}
	else
	{
		// No player controller yet — create with World as outer.
		ActiveLoadingScreen = CreateWidget<UUserWidget>(World, LoadingScreenClass);
	}

	if (ActiveLoadingScreen)
	{
		ActiveLoadingScreen->AddToViewport(100); // ZOrder 100 — always on top

		// Call the BP "StartFadeIn" event on the widget.
		FName FadeInFn = FName("StartFadeIn");
		if (UFunction* Fn = ActiveLoadingScreen->FindFunction(FadeInFn))
			ActiveLoadingScreen->ProcessEvent(Fn, nullptr);

		UE_LOG(LogTemp, Log, TEXT("LevelStreamingManager: loading screen shown."));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("LevelStreamingManager::ShowLoadingScreen — CreateWidget returned null!"));
	}
}

void ULevelStreamingManager::HideLoadingScreen()
{
	if (!ActiveLoadingScreen) return;

	// Call the BP "StartFadeOut" event — widget animates out and removes itself.
	FName FadeOutFn = FName("StartFadeOut");
	if (UFunction* Fn = ActiveLoadingScreen->FindFunction(FadeOutFn))
		ActiveLoadingScreen->ProcessEvent(Fn, nullptr);

	// Null our reference — the widget removes itself from parent when animation ends.
	ActiveLoadingScreen = nullptr;

	UE_LOG(LogTemp, Log, TEXT("LevelStreamingManager: loading screen fade-out started."));
}

// ─────────────────────────────────────────────────────────────────────────────
// PORTAL START TELEPORT
// ─────────────────────────────────────────────────────────────────────────────

void ULevelStreamingManager::TeleportPlayerToPortalStart()
{
	UWorld* World = GetWorld();
	if (!World) return;

	AAlphaExilemetCharacter* Player = GetLocalPlayer();
	if (!Player)
	{
		UE_LOG(LogTemp, Warning, TEXT("LevelStreamingManager::TeleportPlayerToPortalStart — no player."));
		return;
	}

	FName PortalName = LevelToName(CurrentLevel);

	for (ULevelStreaming* SL : World->GetStreamingLevels())
	{
		if (!SL || !SL->IsLevelLoaded()) continue;
		if (!SL->GetWorldAssetPackageFName().ToString().EndsWith(PortalName.ToString())) continue;

		ULevel* Level = SL->GetLoadedLevel();
		if (!Level) continue;

		for (AActor* Actor : Level->Actors)
		{
			if (APlayerStart* PS = Cast<APlayerStart>(Actor))
			{
				Player->SetActorTransform(PS->GetActorTransform(), false, nullptr, ETeleportType::TeleportPhysics);
				if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
					PC->SetControlRotation(PS->GetActorRotation());
				UE_LOG(LogTemp, Log, TEXT("LevelStreamingManager: teleported to portal PlayerStart."));
				return;
			}
		}
		UE_LOG(LogTemp, Warning, TEXT("LevelStreamingManager: portal has no PlayerStart!"));
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("LevelStreamingManager: portal level not found in streaming levels."));
}

// ─────────────────────────────────────────────────────────────────────────────
// HELPERS
// ─────────────────────────────────────────────────────────────────────────────

AAlphaExilemetCharacter* ULevelStreamingManager::GetLocalPlayer() const
{
	UWorld* World = GetWorld();
	if (!World) return nullptr;
	APlayerController* PC = World->GetFirstPlayerController();
	return PC ? Cast<AAlphaExilemetCharacter>(PC->GetPawn()) : nullptr;
}

void ULevelStreamingManager::DestroyPlayerPawn()
{
	if (AAlphaExilemetCharacter* Player = GetLocalPlayer())
	{
		if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
			PC->UnPossess();
		Player->Destroy();
	}
}