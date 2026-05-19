#include "AlphaStreamingSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "AlphaExilemetGameInstance.h"
#include "AlphaExilemet/AlphaExilemetCharacter.h"
#include "AlphaExilemet/Tools/ToolBase.h"
#include "Engine/LevelStreaming.h"
#include "GameFramework/PlayerStart.h"

namespace
{
	AAlphaExilemetCharacter* GetLocalPlayer(UWorld* World)
	{
		if (!World) return nullptr;
		APlayerController* PC = World->GetFirstPlayerController();
		return PC ? Cast<AAlphaExilemetCharacter>(PC->GetPawn()) : nullptr;
	}
}

void UAlphaStreamingSubsystem::StreamLevel(FName LevelToLoad, FName LevelToUnload)
{
	UWorld* World = GetWorld();
	if (!World) return;

	if (!LevelToLoad.IsNone())
	{
		FLatentActionInfo LoadInfo;
		LoadInfo.CallbackTarget    = this;
		LoadInfo.ExecutionFunction = FName("OnStreamLevelLoaded");
		LoadInfo.Linkage           = 0;
		LoadInfo.UUID              = ++LoadLatentUUID;
		UGameplayStatics::LoadStreamLevel(World, LevelToLoad, true, true, LoadInfo);
	}

	if (!LevelToUnload.IsNone())
	{
		FLatentActionInfo UnloadInfo;
		UnloadInfo.CallbackTarget    = this;
		UnloadInfo.ExecutionFunction = FName("OnStreamLevelUnloaded");
		UnloadInfo.Linkage           = 0;
		UnloadInfo.UUID              = ++UnloadLatentUUID;
		UGameplayStatics::UnloadStreamLevel(World, LevelToUnload, UnloadInfo, false);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// RETURN TO MAIN MENU
// ─────────────────────────────────────────────────────────────────────────────

void UAlphaStreamingSubsystem::ReturnToMainMenu()
{
	UAlphaExilemetGameInstance* GI = Cast<UAlphaExilemetGameInstance>(GetGameInstance());

	FName LevelToUnload = CurrentActiveLevel;

	if (GI) GI->CurrentPhase = EGamePhase::MainMenu;
	CurrentActiveLevel = FName("MainMenu");

	// Fires before I/O — GM shows loading screen, destroys player, clears refs.
	OnReturnToMainMenuStarted.Broadcast();

	FName SafeUnload = (LevelToUnload.IsNone() || LevelToUnload == FName("MainMenu"))
		? NAME_None : LevelToUnload;

	UE_LOG(LogTemp, Log,
		TEXT("ReturnToMainMenu: unloading '%s', loading 'MainMenu'."),
		*LevelToUnload.ToString());

	StreamLevel(FName("MainMenu"), SafeUnload);
}

// ─────────────────────────────────────────────────────────────────────────────
// TUTORIAL → MAIN
//
// CurrentPhase is set to Main BEFORE firing OnTutorialToMainComplete.
// SpawnNewGamePlayer reads CurrentPhase to decide what to do — because it
// reads Main (not NewGame_Tutorial) it will call InitializeWakeUp instead
// of InitializeTutorial. This prevents the Tutorial from re-starting.
// ─────────────────────────────────────────────────────────────────────────────

void UAlphaStreamingSubsystem::HandleTutorialCompletion()
{
	// 1. Clear every tool from the player — authoritative clear.
	if (AAlphaExilemetCharacter* Player = GetLocalPlayer(GetWorld()))
	{
		for (AToolBase* Tool : Player->OwnedTools)
			if (Tool) Tool->Destroy();

		Player->OwnedTools.Empty();
		Player->CurrentTool      = nullptr;
		Player->ActiveToolIndex  = -1;
		Player->PendingToolIndex = -1;

		Player->OnInventoryUpdated.Broadcast();
		Player->OnToolWielded.Broadcast(-1);

		UE_LOG(LogTemp, Log, TEXT("HandleTutorialCompletion: all tools destroyed."));
	}

	// 2. Set phase BEFORE streaming so SpawnNewGamePlayer reads the right phase.
	if (UAlphaExilemetGameInstance* GI = Cast<UAlphaExilemetGameInstance>(GetGameInstance()))
	{
		GI->CurrentPhase = EGamePhase::Main;

		if (GI->LocalSaveRef)
			GI->LocalSaveRef->CurrentLevelName = FName("Main");
		GI->SavePlayerData();
	}

	// 3. Stream Main, unload Tutorial.
	bTutorialTransition = true;
	CurrentActiveLevel  = FName("Main");

	StreamLevel(FName("Main"), FName("Tutorial"));

	UE_LOG(LogTemp, Log, TEXT("HandleTutorialCompletion: streaming Main. Phase=Main."));
}

// ─────────────────────────────────────────────────────────────────────────────
// PORTAL — ENTER
// ─────────────────────────────────────────────────────────────────────────────

void UAlphaStreamingSubsystem::EnterPortal(FName PortalLevelName, FTransform PlayerReturnTransform)
{
	UAlphaExilemetGameInstance* GI = Cast<UAlphaExilemetGameInstance>(GetGameInstance());

	OnPortalEnterStarted.Broadcast();

	if (GI && GI->LocalSaveRef)
	{
		GI->LocalSaveRef->PrePortalTransform = PlayerReturnTransform;
		GI->LocalSaveRef->CurrentLevelName   = PortalLevelName;
	}
	if (GI)
	{
		GI->CurrentPhase = EGamePhase::InPortal;
		GI->SavePlayerData();
	}

	FName PreviousLevel = CurrentActiveLevel;
	ActivePortalName    = PortalLevelName;
	CurrentActiveLevel  = PortalLevelName;

	StreamLevel(PortalLevelName, PreviousLevel);

	UE_LOG(LogTemp, Log, TEXT("EnterPortal: loading '%s', unloading '%s'."),
		*PortalLevelName.ToString(), *PreviousLevel.ToString());
}

// ─────────────────────────────────────────────────────────────────────────────
// PORTAL — TELEPORT TO PLAYERSTART
// ─────────────────────────────────────────────────────────────────────────────

void UAlphaStreamingSubsystem::TeleportPlayerToPortalStart()
{
	UWorld* World = GetWorld();
	if (!World) return;

	AAlphaExilemetCharacter* Player = GetLocalPlayer(World);
	if (!Player) { UE_LOG(LogTemp, Warning, TEXT("TeleportPlayerToPortalStart: no player.")); return; }

	for (ULevelStreaming* SL : World->GetStreamingLevels())
	{
		if (!SL || !SL->IsLevelLoaded()) continue;
		if (!SL->GetWorldAssetPackageFName().ToString().EndsWith(ActivePortalName.ToString())) continue;

		ULevel* Level = SL->GetLoadedLevel();
		if (!Level) continue;

		for (AActor* Actor : Level->Actors)
		{
			if (APlayerStart* PS = Cast<APlayerStart>(Actor))
			{
				Player->SetActorTransform(PS->GetActorTransform(), false, nullptr, ETeleportType::TeleportPhysics);
				if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
					PC->SetControlRotation(PS->GetActorRotation());
				UE_LOG(LogTemp, Log, TEXT("TeleportPlayerToPortalStart: done."));
				return;
			}
		}
		UE_LOG(LogTemp, Warning, TEXT("TeleportPlayerToPortalStart: no PlayerStart in portal!"));
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("TeleportPlayerToPortalStart: portal level not found."));
}

// ─────────────────────────────────────────────────────────────────────────────
// PORTAL — EXIT
// ─────────────────────────────────────────────────────────────────────────────

void UAlphaStreamingSubsystem::ExitPortal()
{
	UAlphaExilemetGameInstance* GI = Cast<UAlphaExilemetGameInstance>(GetGameInstance());

	FTransform ReturnTransform;
	bool bHasReturn = false;
	if (GI && GI->LocalSaveRef) { ReturnTransform = GI->LocalSaveRef->PrePortalTransform; bHasReturn = true; }

	AAlphaExilemetCharacter* Player = GetLocalPlayer(GetWorld());

	if (Player && bHasReturn)
	{
		Player->SetActorTransform(ReturnTransform, false, nullptr, ETeleportType::TeleportPhysics);
		if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
			PC->SetControlRotation(ReturnTransform.GetRotation().Rotator());
	}

	if (Player) Player->bIsSurvivalActive = true;

	if (GI)
	{
		GI->CurrentPhase = EGamePhase::Main;
		if (GI->LocalSaveRef) GI->LocalSaveRef->CurrentLevelName = FName("Main");
		GI->SavePlayerData();
	}

	FName PortalToUnload = ActivePortalName;
	ActivePortalName     = NAME_None;
	CurrentActiveLevel   = FName("Main");

	UE_LOG(LogTemp, Log, TEXT("ExitPortal: loading 'Main', unloading '%s'."), *PortalToUnload.ToString());
	StreamLevel(FName("Main"), PortalToUnload);
}

// ─────────────────────────────────────────────────────────────────────────────
// PORTAL — COMPLETE CHALLENGE
// ─────────────────────────────────────────────────────────────────────────────

void UAlphaStreamingSubsystem::CompletePortalChallenge()
{
	if (AAlphaExilemetCharacter* Player = GetLocalPlayer(GetWorld()))
		Player->bIsSurvivalActive = true;

	OnPortalExitStarted.Broadcast();
	UE_LOG(LogTemp, Log, TEXT("CompletePortalChallenge: broadcast."));
}

void UAlphaStreamingSubsystem::Debug_ForceCompleteChallenge()
{
	UE_LOG(LogTemp, Warning, TEXT("Debug_ForceCompleteChallenge called."));
	CompletePortalChallenge();
}

// ─────────────────────────────────────────────────────────────────────────────
// LATENT CALLBACKS
// ─────────────────────────────────────────────────────────────────────────────

void UAlphaStreamingSubsystem::OnStreamLevelLoaded()
{
	if (bTutorialTransition)
	{
		// Tutorial→Main path ONLY fires OnTutorialToMainComplete.
		// OnStreamComplete is intentionally skipped to prevent the GM's
		// Switch-on-Phase from running and double-calling SpawnNewGamePlayer.
		bTutorialTransition = false;
		OnTutorialToMainComplete.Broadcast();
		UE_LOG(LogTemp, Log, TEXT("OnStreamLevelLoaded: OnTutorialToMainComplete broadcast."));
	}
	else
	{
		OnStreamComplete.Broadcast();
		UE_LOG(LogTemp, Log, TEXT("OnStreamLevelLoaded: OnStreamComplete broadcast."));
	}
}

void UAlphaStreamingSubsystem::OnStreamLevelUnloaded()
{
	UE_LOG(LogTemp, Verbose, TEXT("OnStreamLevelUnloaded: silent."));
}
