#include "AlphaStreamingSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "AlphaExilemetGameInstance.h"
#include "AlphaExilemet/AlphaExilemetCharacter.h"
#include "AlphaExilemet/Tools/ToolBase.h"
#include "Engine/LevelStreaming.h"
#include "GameFramework/PlayerStart.h"

// ─────────────────────────────────────────────────────────────────────────────
// HELPERS
// ─────────────────────────────────────────────────────────────────────────────

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
// KEY FIX: Load and unload use DIFFERENT ExecutionFunction names.
//   Load  → "OnStreamLevelLoaded"   → broadcasts OnStreamComplete
//   Unload→ "OnStreamLevelUnloaded" → does nothing (silent)
//
// Previously both used "OnStreamLevelLoaded", so OnStreamComplete was
// broadcast twice — once when load finished and once when unload finished.
// The second broadcast fired with the wrong phase and broke the GM switch.
// ─────────────────────────────────────────────────────────────────────────────

void UAlphaStreamingSubsystem::StreamLevel(FName LevelToLoad, FName LevelToUnload)
{
	UWorld* World = GetWorld();
	if (!World) return;

	// LOAD — callback broadcasts OnStreamComplete when done.
	FLatentActionInfo LoadInfo;
	LoadInfo.CallbackTarget    = this;
	LoadInfo.ExecutionFunction = FName("OnStreamLevelLoaded");   // ← fires broadcast
	LoadInfo.Linkage           = 0;
	LoadInfo.UUID              = ++LoadLatentUUID;
	UGameplayStatics::LoadStreamLevel(World, LevelToLoad, true, true, LoadInfo);

	// UNLOAD — callback is silent; it just needs a valid function to not crash.
	if (!LevelToUnload.IsNone())
	{
		FLatentActionInfo UnloadInfo;
		UnloadInfo.CallbackTarget    = this;
		UnloadInfo.ExecutionFunction = FName("OnStreamLevelUnloaded"); // ← silent
		UnloadInfo.Linkage           = 0;
		UnloadInfo.UUID              = ++UnloadLatentUUID;
		UGameplayStatics::UnloadStreamLevel(World, LevelToUnload, UnloadInfo, false);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// TUTORIAL → MAIN
// ─────────────────────────────────────────────────────────────────────────────

void UAlphaStreamingSubsystem::HandleTutorialCompletion()
{
	// Clear tutorial tools so they don't persist into Main.
	if (AAlphaExilemetCharacter* Player = GetLocalPlayer(GetWorld()))
	{
		for (AToolBase* Tool : Player->OwnedTools)
			if (Tool) Tool->Destroy();

		Player->OwnedTools.Empty();
		Player->CurrentTool     = nullptr;
		Player->ActiveToolIndex = -1;

		// Notify HUD to clear inventory display immediately.
		Player->OnInventoryUpdated.Broadcast();
		Player->OnToolWielded.Broadcast(-1);
	}

	if (UAlphaExilemetGameInstance* GI = Cast<UAlphaExilemetGameInstance>(GetGameInstance()))
	{
		if (GI->LocalSaveRef)
			GI->LocalSaveRef->CurrentLevelName = FName("Main");
		GI->SavePlayerData();
	}

	bTutorialTransition = true;
	StreamLevel(FName("Main"), FName("Tutorial"));
}

// ─────────────────────────────────────────────────────────────────────────────
// PORTAL — ENTER
//
// KEY FIX: Main IS NOW UNLOADED when entering a portal.
// Previously Main was kept loaded and the portal level was loaded on top,
// causing both environments to be visible at the same time.
//
// New sequence:
//   1. Phase = InPortal  (before everything so GM switch reads it correctly)
//   2. OnPortalEnterStarted → GM creates loading screen immediately
//   3. Save player state + PrePortalTransform to disk
//   4. LoadStreamLevel(portal)  → OnStreamLevelLoaded  → OnStreamComplete → GM switch InPortal case
//   5. UnloadStreamLevel(Main)  → OnStreamLevelUnloaded → silent
// ─────────────────────────────────────────────────────────────────────────────

void UAlphaStreamingSubsystem::EnterPortal(FName PortalLevelName, FTransform PlayerReturnTransform)
{
	UAlphaExilemetGameInstance* GI = Cast<UAlphaExilemetGameInstance>(GetGameInstance());

	// ── 1. PHASE ──────────────────────────────────────────────────────────────
	if (GI) GI->CurrentPhase = EGamePhase::InPortal;

	// ── 2. LOADING SCREEN ─────────────────────────────────────────────────────
	// Broadcast BEFORE any I/O so the screen is visible before any frame hitch.
	OnPortalEnterStarted.Broadcast();

	// ── 3. SAVE STATE ─────────────────────────────────────────────────────────
	if (GI && GI->LocalSaveRef)
	{
		// PrePortalTransform already has 180° yaw baked in by APortalBase.
		// ExitPortal() restores it directly — no extra math needed.
		GI->LocalSaveRef->PrePortalTransform = PlayerReturnTransform;
		// Do NOT write portal level name — SavePlayerData() guards this
		// and always writes "Main" to prevent getting stuck in a portal on reload.
	}
	if (GI) GI->SavePlayerData();

	// ── 4+5. STREAM ───────────────────────────────────────────────────────────
	// Load portal level  → OnStreamLevelLoaded fires when ready → OnStreamComplete broadcast
	// Unload Main level  → OnStreamLevelUnloaded fires (silent, no broadcast)
	ActivePortalName = PortalLevelName;
	StreamLevel(PortalLevelName, FName("Main")); // ← Main is now UNLOADED

	UE_LOG(LogTemp, Log,
		TEXT("EnterPortal: loading '%s', unloading 'Main'. Phase = InPortal."),
		*PortalLevelName.ToString());
}

// ─────────────────────────────────────────────────────────────────────────────
// PORTAL — TELEPORT TO PLAYERSTART
//
// Moves the existing pawn to the PlayerStart inside the portal streaming level.
// Called from GM BP InPortal switch case after OnStreamComplete fires.
// ─────────────────────────────────────────────────────────────────────────────

void UAlphaStreamingSubsystem::TeleportPlayerToPortalStart()
{
	UWorld* World = GetWorld();
	if (!World) return;

	AAlphaExilemetCharacter* Player = GetLocalPlayer(World);
	if (!Player)
	{
		UE_LOG(LogTemp, Warning, TEXT("TeleportPlayerToPortalStart: No player pawn found."));
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
					TEXT("TeleportPlayerToPortalStart: teleported to '%s' in '%s'."),
					*PS->GetName(), *PkgName);
				return;
			}
		}

		UE_LOG(LogTemp, Warning,
			TEXT("TeleportPlayerToPortalStart: portal level '%s' found but has no PlayerStart!"),
			*PkgName);
		return;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("TeleportPlayerToPortalStart: no loaded streaming level matches '%s'."),
		*ActivePortalName.ToString());
}

// ─────────────────────────────────────────────────────────────────────────────
// PORTAL — EXIT
//
// Sequence:
//   1. Teleport player to PrePortalTransform (already yaw-rotated by PortalBase)
//   2. Re-enable survival
//   3. Phase = Main  (before load so GM switch reads it correctly)
//   4. LoadStreamLevel(Main)    → OnStreamLevelLoaded → OnStreamComplete → GM fades out screen
//   5. UnloadStreamLevel(portal)→ OnStreamLevelUnloaded → silent
// ─────────────────────────────────────────────────────────────────────────────

void UAlphaStreamingSubsystem::ExitPortal()
{
	UAlphaExilemetGameInstance* GI = Cast<UAlphaExilemetGameInstance>(GetGameInstance());

	// ── 1. RESTORE PLAYER POSITION ───────────────────────────────────────────
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

	// ── 2. RE-ENABLE SURVIVAL ────────────────────────────────────────────────
	if (Player) Player->bIsSurvivalActive = true;

	// ── 3. PHASE ─────────────────────────────────────────────────────────────
	// Set BEFORE streaming so when OnStreamLevelLoaded fires, the GM switch
	// reads "Main" and knows to fade out the loading screen.
	if (GI)
	{
		GI->CurrentPhase = EGamePhase::Main;
		if (GI->LocalSaveRef)
			GI->LocalSaveRef->CurrentLevelName = FName("Main");
		GI->SavePlayerData();
	}

	// ── 4+5. STREAM ───────────────────────────────────────────────────────────
	FName PortalToUnload = ActivePortalName;
	ActivePortalName     = NAME_None;

	UE_LOG(LogTemp, Log,
		TEXT("ExitPortal: loading 'Main', unloading '%s'. Phase = Main."),
		*PortalToUnload.ToString());

	// Load Main  → OnStreamLevelLoaded → OnStreamComplete → GM Main case fades out screen
	// Unload portal → OnStreamLevelUnloaded → silent
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
	// Broadcast the generic "load finished" event.
	// GM's OnAnyLevelStreamComplete is bound here and switches on CurrentPhase.
	OnStreamComplete.Broadcast();

	if (bTutorialTransition)
	{
		bTutorialTransition = false;
		OnTutorialToMainComplete.Broadcast();
	}

	UE_LOG(LogTemp, Log, TEXT("OnStreamLevelLoaded: OnStreamComplete broadcast."));
}

void UAlphaStreamingSubsystem::OnStreamLevelUnloaded()
{
	// Intentionally empty — unloads must never trigger GM logic.
	// This function exists only so the LatentActionInfo has a valid callback.
	UE_LOG(LogTemp, Verbose, TEXT("OnStreamLevelUnloaded: silent."));
}