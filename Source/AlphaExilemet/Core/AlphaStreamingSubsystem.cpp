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

// ─────────────────────────────────────────────────────────────────────────────
// GENERIC STREAM
//
// Load  → OnStreamLevelLoaded   → broadcasts OnStreamComplete (or TutorialToMain)
// Unload→ OnStreamLevelUnloaded → intentionally silent
// ─────────────────────────────────────────────────────────────────────────────

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
//
// Works from ANY active level: Tutorial, Main, or any Portal.
// Uses CurrentActiveLevel to know what to unload — no hardcoding.
// Fires OnReturnToMainMenuStarted BEFORE I/O so the GM can clean up.
// ─────────────────────────────────────────────────────────────────────────────

void UAlphaStreamingSubsystem::ReturnToMainMenu()
{
	UAlphaExilemetGameInstance* GI = Cast<UAlphaExilemetGameInstance>(GetGameInstance());

	// Store the level we need to unload before we overwrite CurrentActiveLevel.
	FName LevelToUnload = CurrentActiveLevel;

	// Update phase immediately so any in-flight delegates read the right state.
	if (GI) GI->CurrentPhase = EGamePhase::MainMenu;

	// Update tracking BEFORE broadcasting so anything reading in the handler
	// already sees "MainMenu".
	CurrentActiveLevel = FName("MainMenu");

	// Signal GM to: show loading screen, destroy player pawn, clear director refs.
	OnReturnToMainMenuStarted.Broadcast();

	UE_LOG(LogTemp, Log,
		TEXT("ReturnToMainMenu: unloading '%s', loading 'MainMenu'."),
		*LevelToUnload.ToString());

	// Load Main Menu, unload whatever was active.
	// If LevelToUnload is "MainMenu" or NAME_None we safely pass NAME_None.
	FName SafeUnload = (LevelToUnload.IsNone() || LevelToUnload == FName("MainMenu"))
		? NAME_None
		: LevelToUnload;

	StreamLevel(FName("MainMenu"), SafeUnload);
}

// ─────────────────────────────────────────────────────────────────────────────
// TUTORIAL → MAIN
// ─────────────────────────────────────────────────────────────────────────────

void UAlphaStreamingSubsystem::HandleTutorialCompletion()
{
	// ── 1. CLEAR ALL PLAYER TOOLS ────────────────────────────────────────────
	// This is the authoritative tool-clear point. The Tutorial Director has
	// already called ClearTutorialPickaxe() for the pickaxe, but we clear
	// everything here as a safety net (in case any other tool was somehow added).
	if (AAlphaExilemetCharacter* Player = GetLocalPlayer(GetWorld()))
	{
		// Destroy every tool actor in the world.
		for (AToolBase* Tool : Player->OwnedTools)
		{
			if (Tool) Tool->Destroy();
		}

		// Clear all inventory state on the character.
		Player->OwnedTools.Empty();
		Player->CurrentTool     = nullptr;
		Player->ActiveToolIndex = -1;
		Player->PendingToolIndex = -1;

		// Notify UI that the inventory is now empty.
		Player->OnInventoryUpdated.Broadcast();
		Player->OnToolWielded.Broadcast(-1);

		UE_LOG(LogTemp, Log, TEXT("HandleTutorialCompletion: All player tools cleared."));
	}

	// ── 2. SAVE WITH MAIN AS TARGET LEVEL ────────────────────────────────────
	if (UAlphaExilemetGameInstance* GI = Cast<UAlphaExilemetGameInstance>(GetGameInstance()))
	{
		if (GI->LocalSaveRef)
			GI->LocalSaveRef->CurrentLevelName = FName("Main");
		GI->SavePlayerData();
	}

	// ── 3. STREAM: load Main, unload Tutorial ────────────────────────────────
	// Mark the flag BEFORE calling StreamLevel so OnStreamLevelLoaded
	// knows to fire OnTutorialToMainComplete instead of OnStreamComplete.
	bTutorialTransition    = true;
	CurrentActiveLevel     = FName("Main");

	StreamLevel(FName("Main"), FName("Tutorial"));

	UE_LOG(LogTemp, Log, TEXT("HandleTutorialCompletion: Streaming Main, unloading Tutorial."));
}

// ─────────────────────────────────────────────────────────────────────────────
// PORTAL — ENTER
// Main IS unloaded. Portal and Main never coexist.
// ─────────────────────────────────────────────────────────────────────────────

void UAlphaStreamingSubsystem::EnterPortal(FName PortalLevelName, FTransform PlayerReturnTransform)
{
	UAlphaExilemetGameInstance* GI = Cast<UAlphaExilemetGameInstance>(GetGameInstance());

	// Show loading screen BEFORE any I/O.
	OnPortalEnterStarted.Broadcast();

	// Save PrePortalTransform so ExitPortal can return the player.
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

	// Store what we're unloading before we update the tracker.
	FName PreviousLevel = CurrentActiveLevel;

	ActivePortalName   = PortalLevelName;
	CurrentActiveLevel = PortalLevelName;

	StreamLevel(PortalLevelName, PreviousLevel);

	UE_LOG(LogTemp, Log,
		TEXT("EnterPortal: loading '%s', unloading '%s'. Phase = InPortal."),
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
	if (!Player)
	{
		UE_LOG(LogTemp, Warning, TEXT("TeleportPlayerToPortalStart: no player pawn."));
		return;
	}

	for (ULevelStreaming* StreamingLevel : World->GetStreamingLevels())
	{
		if (!StreamingLevel || !StreamingLevel->IsLevelLoaded()) continue;

		const FString PkgName = StreamingLevel->GetWorldAssetPackageFName().ToString();
		if (!PkgName.EndsWith(ActivePortalName.ToString())) continue;

		ULevel* Level = StreamingLevel->GetLoadedLevel();
		if (!Level) continue;

		for (AActor* Actor : Level->Actors)
		{
			if (!Actor) continue;
			if (APlayerStart* PS = Cast<APlayerStart>(Actor))
			{
				Player->SetActorTransform(
					PS->GetActorTransform(), false, nullptr, ETeleportType::TeleportPhysics);

				if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
					PC->SetControlRotation(PS->GetActorRotation());

				UE_LOG(LogTemp, Log,
					TEXT("TeleportPlayerToPortalStart: teleported to '%s'."), *PS->GetName());
				return;
			}
		}

		UE_LOG(LogTemp, Warning,
			TEXT("TeleportPlayerToPortalStart: portal '%s' has no PlayerStart!"),
			*ActivePortalName.ToString());
		return;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("TeleportPlayerToPortalStart: no loaded level matches '%s'."),
		*ActivePortalName.ToString());
}

// ─────────────────────────────────────────────────────────────────────────────
// PORTAL — EXIT
// ─────────────────────────────────────────────────────────────────────────────

void UAlphaStreamingSubsystem::ExitPortal()
{
	UAlphaExilemetGameInstance* GI = Cast<UAlphaExilemetGameInstance>(GetGameInstance());

	FTransform ReturnTransform;
	bool bHasReturn = false;

	if (GI && GI->LocalSaveRef)
	{
		ReturnTransform = GI->LocalSaveRef->PrePortalTransform;
		bHasReturn      = true;
	}

	UWorld* World = GetWorld();
	AAlphaExilemetCharacter* Player = GetLocalPlayer(World);

	if (Player && bHasReturn)
	{
		Player->SetActorTransform(
			ReturnTransform, false, nullptr, ETeleportType::TeleportPhysics);

		if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
			PC->SetControlRotation(ReturnTransform.GetRotation().Rotator());
	}

	if (Player) Player->bIsSurvivalActive = true;

	if (GI)
	{
		GI->CurrentPhase = EGamePhase::Main;
		if (GI->LocalSaveRef)
			GI->LocalSaveRef->CurrentLevelName = FName("Main");
		GI->SavePlayerData();
	}

	FName PortalToUnload = ActivePortalName;
	ActivePortalName     = NAME_None;
	CurrentActiveLevel   = FName("Main");

	UE_LOG(LogTemp, Log,
		TEXT("ExitPortal: loading 'Main', unloading '%s'. Phase = Main."),
		*PortalToUnload.ToString());

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

	UE_LOG(LogTemp, Log,
		TEXT("CompletePortalChallenge: survival re-enabled, OnPortalExitStarted broadcast."));
}

void UAlphaStreamingSubsystem::Debug_ForceCompleteChallenge()
{
	UE_LOG(LogTemp, Warning, TEXT("Debug_ForceCompleteChallenge: forcing portal exit."));
	CompletePortalChallenge();
}

// ─────────────────────────────────────────────────────────────────────────────
// LATENT CALLBACKS
// ─────────────────────────────────────────────────────────────────────────────

void UAlphaStreamingSubsystem::OnStreamLevelLoaded()
{
	if (bTutorialTransition)
	{
		// Tutorial→Main path: fire the dedicated delegate ONLY.
		// The GM binds SpawnNewGamePlayer to OnTutorialToMainComplete.
		// OnStreamComplete is intentionally NOT fired here — that would
		// cause the GM's EGamePhase switch to run and double-spawn the player.
		bTutorialTransition = false;
		OnTutorialToMainComplete.Broadcast();
		UE_LOG(LogTemp, Log, TEXT("OnStreamLevelLoaded: OnTutorialToMainComplete broadcast."));
	}
	else
	{
		// All other streaming paths (portal enter, portal exit, load-save, etc.)
		OnStreamComplete.Broadcast();
		UE_LOG(LogTemp, Log, TEXT("OnStreamLevelLoaded: OnStreamComplete broadcast."));
	}
}

void UAlphaStreamingSubsystem::OnStreamLevelUnloaded()
{
	// Intentionally empty — unloads must never trigger GM routing logic.
	UE_LOG(LogTemp, Verbose, TEXT("OnStreamLevelUnloaded: silent."));
}
