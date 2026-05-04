#include "AlphaStreamingSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "AlphaExilemetGameInstance.h"

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
		UnloadInfo.ExecutionFunction = FName("OnStreamLevelLoaded"); // Not used for unload
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
	UAlphaExilemetGameInstance* GI = Cast<UAlphaExilemetGameInstance>(GetGameInstance());
	if (GI && GI->LocalSaveRef)
	{
		// Remember where the player was standing before entering.
		GI->LocalSaveRef->PrePortalTransform = PlayerEntryTransform;
		GI->LocalSaveRef->CurrentLevelName   = PortalLevelName;
		GI->SavePlayerData();
	}

	// Store the active portal name so ExitPortal() knows what to unload.
	ActivePortalName = PortalLevelName;

	// DESIGN DECISION: Main is NEVER unloaded while inside a portal.
	// Pass NAME_None so StreamLevel does NOT unload anything.
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
		ReturnTransform     = GI->LocalSaveRef->PrePortalTransform;
		bHasReturnTransform = true;

		// Reset save state back to Main.
		GI->LocalSaveRef->CurrentLevelName = FName("Main");
		GI->SavePlayerData();
	}

	// Restore player to pre-portal position.
	if (bHasReturnTransform)
	{
		if (UWorld* World = GetWorld())
		{
			if (AAlphaExilemetCharacter* Player = Cast<AAlphaExilemetCharacter>(
				World->GetFirstPlayerController() ? World->GetFirstPlayerController()->GetPawn() : nullptr))
			{
				Player->SetActorTransform(ReturnTransform, false, nullptr, ETeleportType::TeleportPhysics);
			}
		}
	}

	// DESIGN DECISION: Main was never unloaded; we only need to unload the portal.
	// No load call needed — Main is already resident.
	FName PortalToUnload = ActivePortalName;
	ActivePortalName     = NAME_None;

	if (!PortalToUnload.IsNone())
	{
		UWorld* World = GetWorld();
		if (!World) return;

		FLatentActionInfo UnloadInfo;
		UnloadInfo.CallbackTarget    = this;
		UnloadInfo.ExecutionFunction = FName("OnStreamLevelLoaded");
		UnloadInfo.Linkage           = 0;
		UnloadInfo.UUID              = ++LatentUUID;
		UGameplayStatics::UnloadStreamLevel(World, PortalToUnload, UnloadInfo, false);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// LATENT CALLBACK
// ─────────────────────────────────────────────────────────────────────────────

void UAlphaStreamingSubsystem::OnStreamLevelLoaded()
{
	// Always broadcast the generic "something finished loading" event.
	OnStreamComplete.Broadcast();

	// If this was the Tutorial→Main transition, fire the specific delegate
	// that WB_TutorialBlackout is bound to, then reset the flag.
	if (bTutorialTransition)
	{
		bTutorialTransition = false;
		OnTutorialToMainComplete.Broadcast();
	}

	UE_LOG(LogTemp, Log, TEXT("UAlphaStreamingSubsystem: Level stream complete. TutorialTransition was %s."),
		bTutorialTransition ? TEXT("true") : TEXT("false"));
}
