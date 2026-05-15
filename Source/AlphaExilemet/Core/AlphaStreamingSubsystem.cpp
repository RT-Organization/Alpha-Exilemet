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
// ─────────────────────────────────────────────────────────────────────────────

void UAlphaStreamingSubsystem::StreamLevel(FName LevelToLoad, FName LevelToUnload)
{
	UWorld* World = GetWorld();
	if (!World) return;

	FLatentActionInfo LatentInfo;
	LatentInfo.CallbackTarget    = this;
	LatentInfo.ExecutionFunction = FName("OnStreamLevelLoaded");
	LatentInfo.Linkage           = 0;
	LatentInfo.UUID              = ++LatentUUID;
	UGameplayStatics::LoadStreamLevel(World, LevelToLoad, true, true, LatentInfo);

	if (!LevelToUnload.IsNone())
	{
		FLatentActionInfo UnloadInfo;
		UnloadInfo.CallbackTarget    = this;
		UnloadInfo.ExecutionFunction = FName("OnStreamLevelLoaded");
		UnloadInfo.Linkage           = 0;
		UnloadInfo.UUID              = ++LatentUUID;
		UGameplayStatics::UnloadStreamLevel(World, LevelToUnload, UnloadInfo, false);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// TUTORIAL → MAIN
// ─────────────────────────────────────────────────────────────────────────────

void UAlphaStreamingSubsystem::HandleTutorialCompletion()
{
	// Clear tutorial tools before saving so they don't persist into Main.
	if (AAlphaExilemetCharacter* Player = GetLocalPlayer(GetWorld()))
	{
		for (AToolBase* Tool : Player->OwnedTools)
			if (Tool) Tool->Destroy();
		Player->OwnedTools.Empty();
		Player->CurrentTool     = nullptr;
		Player->ActiveToolIndex = -1;
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
// ─────────────────────────────────────────────────────────────────────────────

void UAlphaStreamingSubsystem::EnterPortal(FName PortalLevelName, FTransform PlayerReturnTransform)
{
	UAlphaExilemetGameInstance* GI = Cast<UAlphaExilemetGameInstance>(GetGameInstance());

	// ── 1. SET PHASE FIRST ───────────────────────────────────────────────────
	// Must be set before broadcasting so if the GM BP reads phase inside the
	// OnPortalEnterStarted handler, it already sees InPortal.
	if (GI)
	{
		GI->CurrentPhase = EGamePhase::InPortal;
	}

	// ── 2. SHOW LOADING SCREEN ───────────────────────────────────────────────
	// Broadcast BEFORE any streaming or saving so the screen is up before any
	// frame hitch from I/O. GM BP creates WB_LoadingScreen here.
	OnPortalEnterStarted.Broadcast();

	// ── 3. PERSIST STATE ─────────────────────────────────────────────────────
	// PlayerReturnTransform already has the 180° yaw baked in by APortalBase
	// so ExitPortal() can restore it directly with no extra math.
	if (GI && GI->LocalSaveRef)
	{
		GI->LocalSaveRef->PrePortalTransform = PlayerReturnTransform;
		GI->LocalSaveRef->CurrentLevelName   = PortalLevelName;
	}
	if (GI)
	{
		GI->SavePlayerData(); // captures health, oxygen, tools, currency
	}

	// ── 4. STREAM ────────────────────────────────────────────────────────────
	// Main is NEVER unloaded while inside a portal.
	// OnStreamComplete fires when the load finishes → GM InPortal switch case runs.
	ActivePortalName = PortalLevelName;
	StreamLevel(PortalLevelName, NAME_None);

	UE_LOG(LogTemp, Log, TEXT("UAlphaStreamingSubsystem::EnterPortal — streaming '%s'. Phase = InPortal."),
		*PortalLevelName.ToString());
}

// ─────────────────────────────────────────────────────────────────────────────
// PORTAL — TELEPORT TO PLAYERSTART IN PORTAL LEVEL
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

	// Iterate through ALL streaming levels and find the one that matches the
	// active portal name, then grab its PlayerStart.
	for (ULevelStreaming* StreamingLevel : World->GetStreamingLevels())
	{
		if (!StreamingLevel || !StreamingLevel->IsLevelLoaded()) continue;

		// The package name is the full path; we match by checking if it ends
		// with the portal level name (e.g. ".../Portal_Desert").
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
				{
					PC->SetControlRotation(PS->GetActorRotation());
				}

				UE_LOG(LogTemp, Log,
					TEXT("TeleportPlayerToPortalStart: Teleported to '%s' in '%s'."),
					*PS->GetName(), *PkgName);
				return;
			}
		}

		UE_LOG(LogTemp, Warning,
			TEXT("TeleportPlayerToPortalStart: Level '%s' matched but has no PlayerStart!"),
			*PkgName);
		return;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("TeleportPlayerToPortalStart: No loaded streaming level matches portal name '%s'."),
		*ActivePortalName.ToString());
}

// ─────────────────────────────────────────────────────────────────────────────
// PORTAL — EXIT
// ─────────────────────────────────────────────────────────────────────────────

void UAlphaStreamingSubsystem::ExitPortal()
{
	UAlphaExilemetGameInstance* GI = Cast<UAlphaExilemetGameInstance>(GetGameInstance());

	// ── 1. RESTORE TRANSFORM ─────────────────────────────────────────────────
	FTransform ReturnTransform;
	bool bHasReturn = false;

	if (GI && GI->LocalSaveRef)
	{
		ReturnTransform = GI->LocalSaveRef->PrePortalTransform;
		bHasReturn      = true;

		GI->LocalSaveRef->CurrentLevelName = FName("Main");
		GI->SavePlayerData(); // save current state (tools acquired in portal, etc.)
	}

	UWorld* World = GetWorld();
	AAlphaExilemetCharacter* Player = GetLocalPlayer(World);

	if (Player && bHasReturn)
	{
		Player->SetActorTransform(
			ReturnTransform, false, nullptr, ETeleportType::TeleportPhysics);

		if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
		{
			PC->SetControlRotation(ReturnTransform.GetRotation().Rotator());
		}
	}

	// ── 2. RE-ENABLE SURVIVAL ────────────────────────────────────────────────
	if (Player)
	{
		Player->bIsSurvivalActive = true;
	}

	// ── 3. SET PHASE TO MAIN ─────────────────────────────────────────────────
	// Do this BEFORE the unload so that when OnStreamLevelLoaded fires
	// (after unload completes), the GM switch sees "Main" and fades out
	// the loading screen correctly.
	if (GI)
	{
		GI->CurrentPhase = EGamePhase::Main;
	}

	// ── 4. UNLOAD PORTAL LEVEL ───────────────────────────────────────────────
	FName PortalToUnload = ActivePortalName;
	ActivePortalName     = NAME_None;

	if (!PortalToUnload.IsNone() && World)
	{
		FLatentActionInfo UnloadInfo;
		UnloadInfo.CallbackTarget    = this;
		UnloadInfo.ExecutionFunction = FName("OnStreamLevelLoaded");
		UnloadInfo.Linkage           = 0;
		UnloadInfo.UUID              = ++LatentUUID;
		UGameplayStatics::UnloadStreamLevel(World, PortalToUnload, UnloadInfo, false);

		UE_LOG(LogTemp, Log,
			TEXT("UAlphaStreamingSubsystem::ExitPortal — unloading '%s'. Phase = Main."),
			*PortalToUnload.ToString());
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// PORTAL — COMPLETE CHALLENGE (called by the other programmer)
// ─────────────────────────────────────────────────────────────────────────────

void UAlphaStreamingSubsystem::CompletePortalChallenge()
{
	// Re-enable survival before broadcasting so any end-of-challenge widgets
	// that check bIsSurvivalActive already see the correct value.
	if (AAlphaExilemetCharacter* Player = GetLocalPlayer(GetWorld()))
	{
		Player->bIsSurvivalActive = true;
	}

	// Signal the GM BP:
	//   → Create WB_LoadingScreen + fade in
	//   → Delay (match fade-in duration ~0.4s)
	//   → Call ExitPortal()        (C++ teleports + unloads)
	//   → OnStreamComplete fires   (unload done)
	//   → GM Main switch case      → fade out loading screen
	OnPortalExitStarted.Broadcast();

	UE_LOG(LogTemp, Log,
		TEXT("UAlphaStreamingSubsystem::CompletePortalChallenge — survival re-enabled, OnPortalExitStarted broadcast."));
}

// ─────────────────────────────────────────────────────────────────────────────
// DEBUG
// ─────────────────────────────────────────────────────────────────────────────

void UAlphaStreamingSubsystem::Debug_ForceCompleteChallenge()
{
	UE_LOG(LogTemp, Warning,
		TEXT("UAlphaStreamingSubsystem::Debug_ForceCompleteChallenge — forcing portal exit."));
	CompletePortalChallenge();
}

// ─────────────────────────────────────────────────────────────────────────────
// LATENT CALLBACK
// ─────────────────────────────────────────────────────────────────────────────

void UAlphaStreamingSubsystem::OnStreamLevelLoaded()
{
	// Always broadcast the generic event.
	// GM BP's OnAnyLevelStreamComplete binds here and switches on CurrentPhase.
	OnStreamComplete.Broadcast();

	if (bTutorialTransition)
	{
		bTutorialTransition = false;
		OnTutorialToMainComplete.Broadcast();
	}

	UE_LOG(LogTemp, Log,
		TEXT("UAlphaStreamingSubsystem::OnStreamLevelLoaded fired. TutorialTransition=%s."),
		bTutorialTransition ? TEXT("true") : TEXT("false"));
}