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
#include "Camera/CameraActor.h"
#include "Camera/PlayerCameraManager.h"
#include "CineCameraActor.h"

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

	CurrentLevel          = EGameLevel::MainMenu;
	bTransitionInProgress = false;
	bIsPortalExit         = false;

	UE_LOG(LogTemp, Log, TEXT("LevelStreamingManager: initialized at MainMenu."));
}

// ─────────────────────────────────────────────────────────────────────────────
// DIRECT PLAY (Developer PIE directly into Main or any sublevel)
//
// Called from GM InitializeInstance when the current level is NOT L_Persistent.
//
// This sets CurrentLevel = Main and schedules a one-frame timer that broadcasts
// OnLevelTransitionComplete(Main).  The one-frame delay is critical: it lets
// InitializeInstance finish binding OnLevelTransitionComplete BEFORE the event
// fires, so the GM's handler is already wired up when it arrives.
//
// The GM's OnLevelTransitionComplete_Handler hits the Main pin, reads
// bIsPortalExit = false, and calls SpawnNewGamePlayer normally.
// Since CurrentPhase = None (never set), survival stays OFF.
// ─────────────────────────────────────────────────────────────────────────────

void ULevelStreamingManager::InitializeForDirectPlay()
{
	CurrentLevel          = EGameLevel::Main;
	bTransitionInProgress = false;
	bIsPortalExit         = false;

	// Schedule the broadcast for the next frame.
	// Using a minimal positive time so the timer actually fires on the next tick.
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().SetTimer(
			DirectPlayTimerHandle,
			this,
			&ULevelStreamingManager::DirectPlayTimerCallback,
			0.05f,   // one frame at 20fps; safe for any frame rate
			false);
	}

	UE_LOG(LogTemp, Log,
		TEXT("LevelStreamingManager: InitializeForDirectPlay — scheduled OnLevelTransitionComplete(Main)."));
}

void ULevelStreamingManager::DirectPlayTimerCallback()
{
	UE_LOG(LogTemp, Log,
		TEXT("LevelStreamingManager: DirectPlayTimerCallback — broadcasting OnLevelTransitionComplete(Main)."));
	OnLevelTransitionComplete.Broadcast(EGameLevel::Main);
}

// ─────────────────────────────────────────────────────────────────────────────
// START NEW GAME
// ─────────────────────────────────────────────────────────────────────────────

void ULevelStreamingManager::StartNewGame(const FString& SlotName, bool bSeamless)
{
	if (bTransitionInProgress)
	{
		UE_LOG(LogTemp, Warning, TEXT("LevelStreamingManager::StartNewGame — transition in progress."));
		return;
	}

	UAlphaExilemetGameInstance* GI = Cast<UAlphaExilemetGameInstance>(GetGameInstance());
	if (!GI) return;

	GI->CreateNewGame(SlotName);

	UE_LOG(LogTemp, Log, TEXT("LevelStreamingManager::StartNewGame — slot='%s', seamless=%s."),
		*SlotName, bSeamless ? TEXT("true") : TEXT("false"));

	BeginTransition(EGameLevel::Tutorial, EGameLevel::MainMenu, bSeamless);
}

// ─────────────────────────────────────────────────────────────────────────────
// LOAD SAVED GAME
// ─────────────────────────────────────────────────────────────────────────────

void ULevelStreamingManager::LoadSavedGame(const FString& SlotName)
{
	if (bTransitionInProgress)
	{
		UE_LOG(LogTemp, Warning, TEXT("LevelStreamingManager::LoadSavedGame — transition in progress."));
		return;
	}

	if (!UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		UE_LOG(LogTemp, Error,
			TEXT("LevelStreamingManager::LoadSavedGame — no save in slot '%s'."), *SlotName);
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

	const FName SavedLevelName = GI->LocalSaveRef->CurrentLevelName;
	EGameLevel  TargetLevel    = NameToLevel(SavedLevelName);

	if (TargetLevel == EGameLevel::None
		|| TargetLevel == EGameLevel::Tutorial
		|| TargetLevel == EGameLevel::MainMenu)
	{
		TargetLevel = EGameLevel::Tutorial;
		GI->LocalSaveRef->bHasValidTransform = false;
		GI->CurrentPhase = EGamePhase::NewGame_Tutorial;
		UE_LOG(LogTemp, Log, TEXT("LevelStreamingManager::LoadSavedGame — Tutorial restart."));
	}
	else if (IsPortalLevel(TargetLevel))
	{
		GI->CurrentPhase = EGamePhase::InPortal;
		PortalReturnFrom = EGameLevel::Main;
		UE_LOG(LogTemp, Log, TEXT("LevelStreamingManager::LoadSavedGame — Portal save '%s'."),
			*LevelToName(TargetLevel).ToString());
	}
	else
	{
		GI->CurrentPhase = EGamePhase::LoadedGame;
		TargetLevel      = EGameLevel::Main;
		UE_LOG(LogTemp, Log, TEXT("LevelStreamingManager::LoadSavedGame — Main LoadedGame."));
	}

	BeginTransition(TargetLevel, EGameLevel::MainMenu);
}

// ─────────────────────────────────────────────────────────────────────────────
// RETURN TO MAIN MENU
// ─────────────────────────────────────────────────────────────────────────────

void ULevelStreamingManager::ReturnToMainMenu()
{
	if (bTransitionInProgress)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("LevelStreamingManager::ReturnToMainMenu — transition in progress."));
		return;
	}

	// Cancel any pending portal exit delay so it doesn't fire mid-return.
	if (GetWorld())
		GetWorld()->GetTimerManager().ClearTimer(PortalExitDelayHandle);

	EGameLevel FromLevel = CurrentLevel;
	UE_LOG(LogTemp, Log, TEXT("LevelStreamingManager::ReturnToMainMenu — from '%s'."),
		*LevelToName(FromLevel).ToString());

	BeginTransition(EGameLevel::MainMenu, FromLevel, false);
}

// ─────────────────────────────────────────────────────────────────────────────
// TUTORIAL → MAIN
// ─────────────────────────────────────────────────────────────────────────────

void ULevelStreamingManager::CompleteTutorialAndLoadMain()
{
	if (bTransitionInProgress) return;

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

	if (UAlphaExilemetGameInstance* GI = Cast<UAlphaExilemetGameInstance>(GetGameInstance()))
	{
		GI->CurrentPhase = EGamePhase::NewGame_Tutorial;
		if (GI->LocalSaveRef)
			GI->LocalSaveRef->CurrentLevelName = FName("Main");
		GI->SavePlayerData();
	}

	UE_LOG(LogTemp, Log, TEXT("LevelStreamingManager::CompleteTutorialAndLoadMain."));

	BeginTransition(EGameLevel::Main, EGameLevel::Tutorial, true);
}

// ─────────────────────────────────────────────────────────────────────────────
// PORTAL ENTER
//
// The player pawn is NOT destroyed here. The GM's OnLevelTransitionStarted
// handler checks IsPortalLevel(ToLevel) and skips the destroy-pawn path so the
// existing pawn is kept alive throughout the streaming swap.  Once the portal
// level loads, OnLevelTransitionComplete(Portal_X) fires and the GM calls
// TeleportPlayerToPortalStart() to place the pawn at the portal's PlayerStart.
// ─────────────────────────────────────────────────────────────────────────────

void ULevelStreamingManager::EnterPortal(EGameLevel PortalLevel, FTransform PlayerReturnTransform)
{
	if (bTransitionInProgress) return;
	if (!IsPortalLevel(PortalLevel)) return;

	PortalReturnFrom      = CurrentLevel;   // remember Main (or wherever we came from)
	PortalReturnTransform = PlayerReturnTransform;
	bIsPortalExit         = false;          // entering, not exiting

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

	// Disable survival NOW so it doesn't tick during the transition.
	if (AAlphaExilemetCharacter* Player = GetLocalPlayer())
		Player->bIsSurvivalActive = false;

	UE_LOG(LogTemp, Log, TEXT("LevelStreamingManager::EnterPortal — '%s'."),
		*LevelToName(PortalLevel).ToString());

	BeginTransition(PortalLevel, PortalReturnFrom);
}

// ─────────────────────────────────────────────────────────────────────────────
// PORTAL EXIT
//
// KEY DESIGN DECISION — why the teleport is deferred:
//
//   ExitPortal() is called while the portal level is still loaded and Main is
//   not. If we teleport the player NOW:
//     (a) The portal arch's overlap volume is still active and immediately
//         re-fires OnOverlapBegin, sending the player back in.
//     (b) Physics has no valid ground in Main yet so the pawn falls or
//         snaps to origin.
//
//   Solution: do NOT touch the player here. Just update phase/save and kick off
//   BeginTransition.  OnBothConditionsMet() runs only after Main is fully loaded
//   AND MinLoadingScreenTime has elapsed — at that point it is safe to teleport.
//
//   bIsPortalExit is set to true so the GM's OnLevelTransitionComplete handler
//   can take the lightweight "portal exit" path instead of SpawnNewGamePlayer.
// ─────────────────────────────────────────────────────────────────────────────

void ULevelStreamingManager::ExitPortal()
{
	if (bTransitionInProgress) return;

	EGameLevel FromPortal = CurrentLevel;
	bIsPortalExit         = true;   // tell the GM not to spawn a new player

	if (UAlphaExilemetGameInstance* GI = Cast<UAlphaExilemetGameInstance>(GetGameInstance()))
	{
		GI->CurrentPhase = EGamePhase::Main;
		if (GI->LocalSaveRef)
		{
			// Re-confirm the return transform from the save (authoritative source).
			PortalReturnTransform              = GI->LocalSaveRef->PrePortalTransform;
			GI->LocalSaveRef->CurrentLevelName = FName("Main");
		}
		GI->SavePlayerData();
	}

	// PortalReturnFrom was set in EnterPortal() and must NOT be reset here.
	// OnBothConditionsMet reads it to decide whether to apply the return teleport.

	UE_LOG(LogTemp, Log,
		TEXT("LevelStreamingManager::ExitPortal — from '%s'. Teleport deferred until Main loads."),
		*LevelToName(FromPortal).ToString());

	BeginTransition(EGameLevel::Main, FromPortal);
}

// ─────────────────────────────────────────────────────────────────────────────
// CHALLENGE COMPLETION — public API
// ─────────────────────────────────────────────────────────────────────────────

void ULevelStreamingManager::CompletePortalChallenge()
{
	OnPortalExitStarted.Broadcast(CurrentLevel, EGameLevel::Main);
	ExitPortal();
}

void ULevelStreamingManager::CompletePortalChallengeWithDelay(float ExploreDelay)
{
	if (bTransitionInProgress)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("LevelStreamingManager::CompletePortalChallengeWithDelay — transition in progress, ignored."));
		return;
	}

	UWorld* World = GetWorld();
	if (!World) return;

	if (World->GetTimerManager().IsTimerActive(PortalExitDelayHandle))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("LevelStreamingManager::CompletePortalChallengeWithDelay — already counting, ignored."));
		return;
	}

	// Broadcast immediately so directors / UI can react (show reward screen etc.).
	OnPortalExitStarted.Broadcast(CurrentLevel, EGameLevel::Main);

	if (ExploreDelay <= 0.f)
	{
		ExitPortal();
		return;
	}

	UE_LOG(LogTemp, Log,
		TEXT("LevelStreamingManager::CompletePortalChallengeWithDelay — player has %.1fs to explore."),
		ExploreDelay);

	World->GetTimerManager().SetTimer(
		PortalExitDelayHandle,
		this,
		&ULevelStreamingManager::PortalExitDelayCallback,
		ExploreDelay,
		false);
}

void ULevelStreamingManager::PortalExitDelayCallback()
{
	UE_LOG(LogTemp, Log, TEXT("LevelStreamingManager: explore delay elapsed — ExitPortal."));
	ExitPortal();
}

void ULevelStreamingManager::Debug_ForceExitPortal()
{
	UE_LOG(LogTemp, Warning, TEXT("LevelStreamingManager: Debug_ForceExitPortal."));
	if (GetWorld())
		GetWorld()->GetTimerManager().ClearTimer(PortalExitDelayHandle);
	ExitPortal();
}

// ─────────────────────────────────────────────────────────────────────────────
// CORE TRANSITION PIPELINE
// ─────────────────────────────────────────────────────────────────────────────

void ULevelStreamingManager::BeginTransition(EGameLevel NewLevel, EGameLevel OldLevel, bool bSeamless)
{
	bTransitionInProgress = true;
	bLevelLoaded          = false;
	bMinTimeElapsed       = false;
	bSeamlessTransition   = bSeamless;
	PendingLevel          = NewLevel;

	UE_LOG(LogTemp, Log, TEXT("LevelStreamingManager::BeginTransition: '%s' → '%s'%s."),
		*LevelToName(OldLevel).ToString(), *LevelToName(NewLevel).ToString(),
		bSeamless ? TEXT(" [SEAMLESS]") : TEXT(""));

	// 1. Unpause — timers and latent actions need game time to tick.
	UGameplayStatics::SetGamePaused(GetWorld(), false);

	// 2. Notify GM BEFORE any I/O so it can clean up the old level.
	//    IMPORTANT: the GM's OnLevelTransitionStarted_Handler must check
	//    IsPortalLevel(ToLevel) and skip pawn destruction when entering a portal.
	OnLevelTransitionStarted.Broadcast(OldLevel, NewLevel);

	// 3. Loading screen (skipped in seamless mode).
	if (!bSeamless)
	{
		ShowLoadingScreen();

		GetWorld()->GetTimerManager().SetTimer(
			MinTimeHandle,
			this,
			&ULevelStreamingManager::OnMinTimeElapsed,
			MinLoadingScreenTime,
			false);
	}
	else
	{
		bMinTimeElapsed = true;
	}

	UWorld* World = GetWorld();
	if (!World) return;

	// 4. Unload old level.
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

	// 5. Load new level.
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
		UE_LOG(LogTemp, Error,
			TEXT("LevelStreamingManager::BeginTransition — NewLevel has no name!"));
		bTransitionInProgress = false;
		return;
	}

	// 6. Update tracking immediately (before any callbacks fire).
	CurrentLevel = NewLevel;
}

// ─────────────────────────────────────────────────────────────────────────────
// CALLBACKS
// ─────────────────────────────────────────────────────────────────────────────

void ULevelStreamingManager::OnLevelLoaded()
{
	UE_LOG(LogTemp, Log, TEXT("LevelStreamingManager: '%s' loaded."),
		*LevelToName(CurrentLevel).ToString());
	bLevelLoaded = true;
	OnBothConditionsMet();
}

void ULevelStreamingManager::OnLevelUnloaded()
{
	UE_LOG(LogTemp, Verbose, TEXT("LevelStreamingManager: level unloaded (silent)."));
}

void ULevelStreamingManager::OnMinTimeElapsed()
{
	UE_LOG(LogTemp, Log, TEXT("LevelStreamingManager: min display time elapsed."));
	bMinTimeElapsed = true;
	OnBothConditionsMet();
}

void ULevelStreamingManager::OnBothConditionsMet()
{
	if (!bLevelLoaded || !bMinTimeElapsed) return;

	UE_LOG(LogTemp, Log, TEXT("LevelStreamingManager: transition complete → '%s'%s."),
		*LevelToName(CurrentLevel).ToString(),
		bSeamlessTransition ? TEXT(" [SEAMLESS]") : TEXT(""));

	// ── MainMenu: reset stale FPS camera ─────────────────────────────────────
	if (CurrentLevel == EGameLevel::MainMenu)
	{
		ForceCameraToMainMenuCineCamera();
	}

	// ── Portal Exit: apply the deferred player teleport ───────────────────────
	//
	// ExitPortal() did NOT touch the player because Main was not loaded yet.
	// Now that Main is fully loaded it is safe to place the player at the
	// pre-portal transform.  We also re-enable survival here in C++ as a
	// safety net — the GM's handler will do the same via SET IsSurvivalActive.
	//
	// The GM's OnLevelTransitionComplete_Handler reads bIsPortalExit and
	// takes the lightweight path (no SpawnNewGamePlayer) so there is no race.
	//
	if (CurrentLevel == EGameLevel::Main && IsPortalLevel(PortalReturnFrom))
	{
		if (AAlphaExilemetCharacter* Player = GetLocalPlayer())
		{
			Player->SetActorTransform(
				PortalReturnTransform, false, nullptr, ETeleportType::TeleportPhysics);

			if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
				PC->SetControlRotation(PortalReturnTransform.GetRotation().Rotator());

			Player->bIsSurvivalActive = true;

			UE_LOG(LogTemp, Log,
				TEXT("LevelStreamingManager: player restored to pre-portal transform (was in '%s')."),
				*LevelToName(PortalReturnFrom).ToString());
		}

		// Reset so this block does not re-fire on the next Main load.
		PortalReturnFrom = EGameLevel::None;
	}

	// ── Finish ────────────────────────────────────────────────────────────────
	if (!bSeamlessTransition)
		HideLoadingScreen();

	bTransitionInProgress = false;
	bSeamlessTransition   = false;

	// Broadcast BEFORE resetting bIsPortalExit so the GM handler can read it.
	OnLevelTransitionComplete.Broadcast(CurrentLevel);

	// Reset portal-exit flag AFTER the broadcast so it is fresh for the next use.
	bIsPortalExit = false;
}

// ─────────────────────────────────────────────────────────────────────────────
// CAMERA — fix stale FPS state when returning to MainMenu
// ─────────────────────────────────────────────────────────────────────────────

void ULevelStreamingManager::ForceCameraToMainMenuCineCamera()
{
	UWorld* World = GetWorld();
	if (!World) return;

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC) return;

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(World, ACineCameraActor::StaticClass(), FoundActors);
	if (FoundActors.Num() == 0)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("LevelStreamingManager::ForceCameraToMainMenuCineCamera — no CineCameraActor found."));
		return;
	}

	ACineCameraActor* CineCam = Cast<ACineCameraActor>(FoundActors[0]);
	if (!CineCam) return;

	if (APlayerCameraManager* CamMgr = PC->PlayerCameraManager)
	{
		FMinimalViewInfo ViewInfo;
		ViewInfo.Location = CineCam->GetActorLocation();
		ViewInfo.Rotation = CineCam->GetActorRotation();
		ViewInfo.FOV      = 90.0f;
		CamMgr->SetDesiredColorScale(FVector(1, 1, 1), 0.0f);
		CamMgr->FillCameraCache(ViewInfo);
	}

	PC->SetViewTargetWithBlend(CineCam, 0.0f);

	UE_LOG(LogTemp, Log,
		TEXT("LevelStreamingManager: camera forced to CineCameraActor '%s'."),
		*CineCam->GetName());
}

// ─────────────────────────────────────────────────────────────────────────────
// LOADING SCREEN
// ─────────────────────────────────────────────────────────────────────────────

void ULevelStreamingManager::ShowLoadingScreen()
{
	if (!LoadingScreenClass)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("LevelStreamingManager::ShowLoadingScreen — LoadingScreenClass not set!"));
		bMinTimeElapsed = true;   // don't stall if there is no screen
		return;
	}

	if (ActiveLoadingScreen.IsValid())
	{
		ActiveLoadingScreen->RemoveFromParent();
		ActiveLoadingScreen = nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("LevelStreamingManager::ShowLoadingScreen — no World!"));
		bMinTimeElapsed = true;
		return;
	}

	APlayerController* PC = World->GetFirstPlayerController();
	UUserWidget* NewScreen = PC
		? CreateWidget<UUserWidget>(PC, LoadingScreenClass)
		: CreateWidget<UUserWidget>(World, LoadingScreenClass);

	if (!NewScreen)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("LevelStreamingManager::ShowLoadingScreen — CreateWidget returned null!"));
		bMinTimeElapsed = true;
		return;
	}

	ActiveLoadingScreen = NewScreen;
	NewScreen->AddToViewport(100);

	if (UFunction* Fn = NewScreen->FindFunction(FName("StartFadeIn")))
		NewScreen->ProcessEvent(Fn, nullptr);

	UE_LOG(LogTemp, Log, TEXT("LevelStreamingManager: loading screen shown."));
}

void ULevelStreamingManager::HideLoadingScreen()
{
	if (!ActiveLoadingScreen.IsValid()) return;

	UUserWidget* Screen = ActiveLoadingScreen.Get();

	if (UFunction* Fn = Screen->FindFunction(FName("StartFadeOut")))
		Screen->ProcessEvent(Fn, nullptr);

	ActiveLoadingScreen = nullptr;

	UE_LOG(LogTemp, Log, TEXT("LevelStreamingManager: loading screen fade-out started."));
}

// ─────────────────────────────────────────────────────────────────────────────
// PORTAL TELEPORT — teleport to PlayerStart inside the active portal sublevel
// Called by GM on every Portal_X pin of OnLevelTransitionComplete_Handler.
// ─────────────────────────────────────────────────────────────────────────────

void ULevelStreamingManager::TeleportPlayerToPortalStart()
{
	UWorld* World = GetWorld();
	if (!World) return;

	AAlphaExilemetCharacter* Player = GetLocalPlayer();
	if (!Player) return;

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
				Player->SetActorTransform(
					PS->GetActorTransform(), false, nullptr, ETeleportType::TeleportPhysics);
				if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
					PC->SetControlRotation(PS->GetActorRotation());
				UE_LOG(LogTemp, Log,
					TEXT("LevelStreamingManager: teleported to portal PlayerStart."));
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