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

	CurrentLevel          = EGameLevel::MainMenu;
	bTransitionInProgress = false;

	UE_LOG(LogTemp, Log, TEXT("LevelStreamingManager: initialized at MainMenu."));
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

	const FName SavedLevelName = GI->LocalSaveRef->CurrentLevelName;
	EGameLevel  TargetLevel    = NameToLevel(SavedLevelName);

	if (TargetLevel == EGameLevel::None || TargetLevel == EGameLevel::Tutorial
		|| TargetLevel == EGameLevel::MainMenu)
	{
		// Tutorial/corrupt/MainMenu save → restart Tutorial fresh.
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
		// "Main" → normal loaded game.
		GI->CurrentPhase = EGamePhase::LoadedGame;
		TargetLevel      = EGameLevel::Main;
		UE_LOG(LogTemp, Log, TEXT("LevelStreamingManager::LoadSavedGame — Main LoadedGame."));
	}

	// Always unload MainMenu — L_Persistent guarantees it is loaded at this point.
	BeginTransition(TargetLevel, EGameLevel::MainMenu);
}

// ─────────────────────────────────────────────────────────────────────────────
// RETURN TO MAIN MENU
//
// BUG FIX: Tutorial→MainMenu was showing loading screen unnecessarily because
// the Tutorial's skull sequence already shows a black screen. We use a seamless
// transition FROM Tutorial so no loading screen appears — the black screen IS
// the visual cover.  From Main and portals we show the loading screen normally.
// ─────────────────────────────────────────────────────────────────────────────

void ULevelStreamingManager::ReturnToMainMenu()
{
	if (bTransitionInProgress)
	{
		UE_LOG(LogTemp, Warning, TEXT("LevelStreamingManager::ReturnToMainMenu — transition in progress."));
		return;
	}

	EGameLevel FromLevel = CurrentLevel;

	// Tutorial→MainMenu: the skull black screen covers the transition.
	// Use seamless so no second loading screen appears on top of it.
	// Main/Portal→MainMenu: use normal loading screen.
	bool bUseSeamless = (FromLevel == EGameLevel::Tutorial);

	UE_LOG(LogTemp, Log, TEXT("LevelStreamingManager::ReturnToMainMenu — from '%s' seamless=%s."),
		*LevelToName(FromLevel).ToString(), bUseSeamless ? TEXT("true") : TEXT("false"));

	BeginTransition(EGameLevel::MainMenu, FromLevel, bUseSeamless);
}

// ─────────────────────────────────────────────────────────────────────────────
// TUTORIAL → MAIN
// ─────────────────────────────────────────────────────────────────────────────

void ULevelStreamingManager::CompleteTutorialAndLoadMain()
{
	if (bTransitionInProgress) return;

	// Clear all player tools.
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

	// Set phase = Main BEFORE streaming so SpawnNewGamePlayer reads it correctly.
	if (UAlphaExilemetGameInstance* GI = Cast<UAlphaExilemetGameInstance>(GetGameInstance()))
	{
		GI->CurrentPhase = EGamePhase::Main;
		if (GI->LocalSaveRef)
			GI->LocalSaveRef->CurrentLevelName = FName("Main");
		GI->SavePlayerData();
	}

	UE_LOG(LogTemp, Log, TEXT("LevelStreamingManager::CompleteTutorialAndLoadMain. Phase=Main."));

	// Tutorial→Main always shows the loading screen (covers the level swap).
	// The skull black screen has already faded BEFORE this is called.
	BeginTransition(EGameLevel::Main, EGameLevel::Tutorial, false);
}

// ─────────────────────────────────────────────────────────────────────────────
// PORTAL ENTER / EXIT
// ─────────────────────────────────────────────────────────────────────────────

void ULevelStreamingManager::EnterPortal(EGameLevel PortalLevel, FTransform PlayerReturnTransform)
{
	if (bTransitionInProgress) return;
	if (!IsPortalLevel(PortalLevel)) return;

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

	if (AAlphaExilemetCharacter* Player = GetLocalPlayer())
		Player->bIsSurvivalActive = false;

	UE_LOG(LogTemp, Log, TEXT("LevelStreamingManager::EnterPortal — '%s'."),
		*LevelToName(PortalLevel).ToString());

	BeginTransition(PortalLevel, PortalReturnFrom);
}

void ULevelStreamingManager::ExitPortal()
{
	if (bTransitionInProgress) return;

	EGameLevel FromPortal = CurrentLevel;

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
	OnPortalExitStarted.Broadcast(CurrentLevel, EGameLevel::Main);
	ExitPortal();
}

void ULevelStreamingManager::Debug_ForceExitPortal()
{
	UE_LOG(LogTemp, Warning, TEXT("LevelStreamingManager: Debug_ForceExitPortal."));
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

	// 1. Notify GM to clean up BEFORE any I/O.
	OnLevelTransitionStarted.Broadcast(OldLevel, NewLevel);

	// 2. Loading screen (skipped in seamless mode).
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
		// Seamless: treat min time as already done.
		bMinTimeElapsed = true;
	}

	UWorld* World = GetWorld();
	if (!World) return;

	// 3. Unload old level.
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

	// 4. Load new level.
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
		return;
	}

	// Update tracking immediately.
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

	if (!bSeamlessTransition)
		HideLoadingScreen();

	bTransitionInProgress = false;
	bSeamlessTransition   = false;

	OnLevelTransitionComplete.Broadcast(CurrentLevel);
}

// ─────────────────────────────────────────────────────────────────────────────
// LOADING SCREEN
//
// BUG FIX: The old code nulled ActiveLoadingScreen immediately after calling
// StartFadeOut. The BP widget then tried to Remove from Parent after its
// animation but the C++ reference was gone — the widget was orphaned.
//
// Fix: we keep a TWeakObjectPtr to track the widget. We null ActiveLoadingScreen
// so we don't double-call HideLoadingScreen, but the widget itself remains valid
// in memory and can call Remove from Parent when its animation finishes.
// ─────────────────────────────────────────────────────────────────────────────

void ULevelStreamingManager::ShowLoadingScreen()
{
	if (!LoadingScreenClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("LevelStreamingManager::ShowLoadingScreen — LoadingScreenClass not set!"));
		// Still mark min time elapsed so we don't get stuck waiting for a screen
		// that was never shown. OnBothConditionsMet will fire when level loads.
		bMinTimeElapsed = true;
		return;
	}

	// If a previous screen is still alive, remove it immediately.
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

	UUserWidget* NewScreen = nullptr;
	APlayerController* PC = World->GetFirstPlayerController();
	if (PC)
		NewScreen = CreateWidget<UUserWidget>(PC, LoadingScreenClass);
	else
		NewScreen = CreateWidget<UUserWidget>(World, LoadingScreenClass);

	if (!NewScreen)
	{
		UE_LOG(LogTemp, Warning, TEXT("LevelStreamingManager::ShowLoadingScreen — CreateWidget returned null!"));
		bMinTimeElapsed = true;
		return;
	}

	ActiveLoadingScreen = NewScreen; // TWeakObjectPtr — widget stays alive in UMG
	NewScreen->AddToViewport(100);

	// Call StartFadeIn BP event.
	if (UFunction* Fn = NewScreen->FindFunction(FName("StartFadeIn")))
		NewScreen->ProcessEvent(Fn, nullptr);

	UE_LOG(LogTemp, Log, TEXT("LevelStreamingManager: loading screen shown."));
}

void ULevelStreamingManager::HideLoadingScreen()
{
	if (!ActiveLoadingScreen.IsValid()) return;

	UUserWidget* Screen = ActiveLoadingScreen.Get();

	// Call StartFadeOut BP event — widget animates out and calls Remove from Parent itself.
	if (UFunction* Fn = Screen->FindFunction(FName("StartFadeOut")))
		Screen->ProcessEvent(Fn, nullptr);

	// Null our weak pointer so we don't call this twice.
	// The widget object itself stays alive until Remove from Parent fires.
	ActiveLoadingScreen = nullptr;

	UE_LOG(LogTemp, Log, TEXT("LevelStreamingManager: loading screen fade-out started."));
}

// ─────────────────────────────────────────────────────────────────────────────
// PORTAL TELEPORT
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
	UE_LOG(LogTemp, Warning, TEXT("LevelStreamingManager: portal level not found."));
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