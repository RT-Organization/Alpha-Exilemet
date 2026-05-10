#include "AlphaStreamingSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "AlphaExilemetGameInstance.h"
#include "AlphaExilemet/AlphaExilemetCharacter.h"

// ─────────────────────────────────────────────────────────────────────────────
// HELPERS — internal player fetch
// ─────────────────────────────────────────────────────────────────────────────

namespace
{
	// Returns the pawn cast to our character type, or nullptr.
	// Used in ExitPortal and CompletePortalChallenge to avoid code duplication.
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

	// Build latent info so LoadStreamLevel fires OnStreamLevelLoaded on completion.
	FLatentActionInfo LatentInfo;
	LatentInfo.CallbackTarget    = this;
	LatentInfo.ExecutionFunction = FName("OnStreamLevelLoaded");
	LatentInfo.Linkage           = 0;
	LatentInfo.UUID              = ++LatentUUID; // Unique per call

	UGameplayStatics::LoadStreamLevel(World, LevelToLoad, true, true, LatentInfo);

	// Unload only if a target was specified (pass NAME_None to skip).
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
// TUTORIAL → MAIN TRANSITION
// ─────────────────────────────────────────────────────────────────────────────

void UAlphaStreamingSubsystem::HandleTutorialCompletion()
{
	// Save before swapping levels so data is not lost.
	if (UAlphaExilemetGameInstance* GI = Cast<UAlphaExilemetGameInstance>(GetGameInstance()))
	{
		if (GI->LocalSaveRef)
		{
			GI->LocalSaveRef->CurrentLevelName = FName("Main");
		}
		GI->SavePlayerData();
	}

	// Flag this as the tutorial-to-main transition so OnStreamLevelLoaded
	// knows to broadcast OnTutorialToMainComplete when Main finishes loading.
	bTutorialTransition = true;

	// Load Main, unload Tutorial.
	StreamLevel(FName("Main"), FName("Tutorial"));
}

// ─────────────────────────────────────────────────────────────────────────────
// PORTAL — ENTER
// ─────────────────────────────────────────────────────────────────────────────

void UAlphaStreamingSubsystem::EnterPortal(FName PortalLevelName, FTransform PlayerEntryTransform)
{
	// ── FIRE ENTER DELEGATE FIRST ────────────────────────────────────────────
	// GameMode BP binds here and creates WB_LoadingScreen BEFORE any streaming.
	// This ensures the screen is up before any hitching from the load.
	OnPortalEnterStarted.Broadcast();

	// ── PERSIST STATE ────────────────────────────────────────────────────────
	if (UAlphaExilemetGameInstance* GI = Cast<UAlphaExilemetGameInstance>(GetGameInstance()))
	{
		if (GI->LocalSaveRef)
		{
			// PlayerEntryTransform already has the ReturnYawOffset baked in
			// by APortalBase::EnterPortalLevel, so ExitPortal() can restore
			// it directly without extra math.
			GI->LocalSaveRef->PrePortalTransform = PlayerEntryTransform;
			GI->LocalSaveRef->CurrentLevelName   = PortalLevelName;
		}

		// SavePlayerData captures health, oxygen, currency, inventory, tools.
		// If the game crashes inside a portal, loading restores the pre-portal state.
		GI->SavePlayerData();
	}

	// Store the active portal name so ExitPortal() knows what to unload.
	ActivePortalName = PortalLevelName;

	// DESIGN DECISION: Main is NEVER unloaded while inside a portal.
	// Pass NAME_None so StreamLevel does NOT unload anything.
	// OnStreamComplete fires when loading finishes → GameMode fades out screen.
	StreamLevel(PortalLevelName, NAME_None);
}

// ─────────────────────────────────────────────────────────────────────────────
// PORTAL — EXIT
// ─────────────────────────────────────────────────────────────────────────────

void UAlphaStreamingSubsystem::ExitPortal()
{
	UAlphaExilemetGameInstance* GI = Cast<UAlphaExilemetGameInstance>(GetGameInstance());
	FTransform ReturnTransform;
	bool bHasReturnTransform = false;

	if (GI && GI->LocalSaveRef)
	{
		// PrePortalTransform was saved by EnterPortal() with the return-yaw
		// already baked in by APortalBase, so no rotation math needed here.
		ReturnTransform     = GI->LocalSaveRef->PrePortalTransform;
		bHasReturnTransform = true;

		// Reset save state back to Main.
		GI->LocalSaveRef->CurrentLevelName = FName("Main");
		GI->SavePlayerData();
	}

	// ── RESTORE PLAYER ───────────────────────────────────────────────────────
	UWorld* World = GetWorld();
	AAlphaExilemetCharacter* Player = GetLocalPlayer(World);

	if (Player)
	{
		// Teleport back to the pre-portal position (already yaw-rotated).
		if (bHasReturnTransform)
		{
			Player->SetActorTransform(
				ReturnTransform, false, nullptr, ETeleportType::TeleportPhysics);

			if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
			{
				PC->SetControlRotation(ReturnTransform.GetRotation().Rotator());
			}
		}

		// ── RE-ENABLE SURVIVAL ───────────────────────────────────────────────
		// Always re-enable — survival was disabled by APortalBase::EnterPortalLevel.
		Player->bIsSurvivalActive = true;
	}

	// ── UNLOAD THE PORTAL LEVEL ──────────────────────────────────────────────
	// Main was never unloaded, so no load call needed — it's already resident.
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
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// PORTAL — COMPLETE CHALLENGE
// ─────────────────────────────────────────────────────────────────────────────

void UAlphaStreamingSubsystem::CompletePortalChallenge()
{
	// ── RE-ENABLE SURVIVAL ───────────────────────────────────────────────────
	// Do this first so any end-of-challenge logic (e.g. a win screen widget
	// that checks bIsSurvivalActive) already sees the correct value.
	if (AAlphaExilemetCharacter* Player = GetLocalPlayer(GetWorld()))
	{
		Player->bIsSurvivalActive = true;
	}

	// ── SIGNAL THE GAMEMODE BP ───────────────────────────────────────────────
	// The GameMode BP must be bound to OnPortalExitStarted.
	// Expected BP sequence:
	//   OnPortalExitStarted fired
	//     → Create WB_LoadingScreen + play fade-in animation
	//     → Delay (match the fade-in duration, e.g. 0.5 s)
	//     → Call ExitPortal()            ← C++ unloads portal, teleports player
	//     → OnStreamComplete fired
	//       → Call StartFadeOutSequence on the loading screen widget
	OnPortalExitStarted.Broadcast();

	UE_LOG(LogTemp, Log,
		TEXT("UAlphaStreamingSubsystem: CompletePortalChallenge — survival re-enabled, OnPortalExitStarted broadcast."));
}

// ─────────────────────────────────────────────────────────────────────────────
// LATENT CALLBACK
// ─────────────────────────────────────────────────────────────────────────────

void UAlphaStreamingSubsystem::OnStreamLevelLoaded()
{
	// Always broadcast the generic "something finished loading" event.
	// WB_LoadingScreen binds here and starts its fade-out animation.
	OnStreamComplete.Broadcast();

	// If this was the Tutorial→Main transition, fire the specific delegate
	// that WB_TutorialBlackout is bound to, then reset the flag.
	if (bTutorialTransition)
	{
		bTutorialTransition = false;
		OnTutorialToMainComplete.Broadcast();
	}

	UE_LOG(LogTemp, Log,
		TEXT("UAlphaStreamingSubsystem: Level stream complete. TutorialTransition was %s."),
		bTutorialTransition ? TEXT("true") : TEXT("false"));
}
