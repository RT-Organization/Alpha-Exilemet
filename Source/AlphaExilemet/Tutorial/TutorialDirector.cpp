#include "TutorialDirector.h"

// Unreal Engine
#include "Kismet/GameplayStatics.h"
#include "Components/SphereComponent.h"
#include "GameFramework/PlayerController.h"

// Project
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "AlphaExilemet/AlphaExilemetCharacter.h"
#include "AlphaExilemet/BaseCamp.h"

// ─────────────────────────────────────────────────────────────────────────────
// CONSTRUCTOR
// ─────────────────────────────────────────────────────────────────────────────

ATutorialDirector::ATutorialDirector()
{
	PrimaryActorTick.bCanEverTick = false;
}

// ─────────────────────────────────────────────────────────────────────────────
// ENGINE OVERRIDES
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::BeginPlay()
{
	Super::BeginPlay();

	// ── IMPORTANT: Do NOT call InitializeTutorial() here. ──────────────────
	//
	// BeginPlay fires at the same frame as level load. The GameMode may not
	// have spawned or possessed the player yet, so GetTutorialPlayer() would
	// return nullptr, the overlap bind would silently fail, and the sequence
	// would play with no camera switch (FP view bug) or crash (Camera Manager
	// Accessed None).
	//
	// Instead, the GameMode calls InitializeTutorial() explicitly after
	// SpawnNewGamePlayer() completes. See GM_SimulatorGamemode → SpawnNewGamePlayer.
	// ─────────────────────────────────────────────────────────────────────────
}

// ─────────────────────────────────────────────────────────────────────────────
// PUBLIC: InitializeTutorial
// Called by GM_SimulatorGamemode after the player is spawned and possessed.
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::InitializeTutorial()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// ── 1. Verify player is valid ──────────────────────────────────────────
	AAlphaExilemetCharacter* Player = GetTutorialPlayer();
	if (!Player)
	{
		UE_LOG(LogTemp, Error,
			TEXT("ATutorialDirector::InitializeTutorial — player not found! "
			     "Make sure GM calls this AFTER SpawnNewGamePlayer."));
		return;
	}

	// ── 2. Find the intro Level Sequence by tag ────────────────────────────
	TArray<AActor*> TaggedActors;
	UGameplayStatics::GetAllActorsWithTag(World, IntroSequenceTag, TaggedActors);

	if (TaggedActors.Num() == 0)
	{
		UE_LOG(LogTemp, Error,
			TEXT("ATutorialDirector::InitializeTutorial — no actor found with tag '%s'. "
			     "Select the LevelSequenceActor in L_Tutorial → Details → Actor → Tags "
			     "and add 'TutorialIntroSeq'."),
			*IntroSequenceTag.ToString());
		return;
	}

	ALevelSequenceActor* SeqActor = Cast<ALevelSequenceActor>(TaggedActors[0]);
	if (!SeqActor)
	{
		UE_LOG(LogTemp, Error,
			TEXT("ATutorialDirector::InitializeTutorial — actor tagged '%s' is not a "
			     "LevelSequenceActor. Check the level outliner."),
			*IntroSequenceTag.ToString());
		return;
	}

	IntroSequencePlayer = SeqActor->GetSequencePlayer();
	if (!IntroSequencePlayer)
	{
		UE_LOG(LogTemp, Error,
			TEXT("ATutorialDirector::InitializeTutorial — LevelSequenceActor has no "
			     "Sequence Player. Check that a Level Sequence asset is assigned to it."));
		return;
	}

	// ── 3. Bind the OnStop callback ────────────────────────────────────────
	// Use AddUniqueDynamic so re-entering this function never double-binds.
	IntroSequencePlayer->OnStop.AddUniqueDynamic(this, &ATutorialDirector::OnIntroSequenceFinished);

	// ── 4. Bind out-of-bounds overlap guard ────────────────────────────────
	// Do this here (not in BeginPlay) so the player is definitely valid
	// when the callback fires.
	if (AActor* BaseCampActor = FindBaseCamp())
	{
		if (ABaseCamp* BaseCamp = Cast<ABaseCamp>(BaseCampActor))
		{
			if (BaseCamp->OxygenSphere)
			{
				BaseCamp->OxygenSphere->OnComponentEndOverlap.AddUniqueDynamic(
					this, &ATutorialDirector::OnOxygenSphereEndOverlap);
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("ATutorialDirector::InitializeTutorial — BaseCamp not found in level. "
			     "Out-of-bounds guard will not be active."));
	}

	// ── 5. Disable player input before the cinematic ──────────────────────
	if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
	{
		FInputModeUIOnly UIMode;
		PC->SetInputMode(UIMode);
	}

	// ── 6. Play the intro sequence ─────────────────────────────────────────
	// The sequence MUST have a Camera Cut track with the CineCameraActor
	// assigned. Without a Camera Cut track, Sequencer will not switch the
	// viewport — this is the most common cause of the "FP view during cinematic"
	// bug and must be fixed in the Sequencer editor, not in code.
	IntroSequencePlayer->Play();

	UE_LOG(LogTemp, Log,
		TEXT("ATutorialDirector: CS_TutorialIntro started. Player input disabled."));
}

// ─────────────────────────────────────────────────────────────────────────────
// PRIVATE: OnIntroSequenceFinished
// Bound to IntroSequencePlayer::OnStop. Fires when Sequencer reaches the end
// or when Stop() is called manually.
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::OnIntroSequenceFinished()
{
	AAlphaExilemetCharacter* Player = GetTutorialPlayer();
	if (!Player) return;

	// ── 1. Capture the player's current world position ─────────────────────
	// At this moment the camera blend from Sequencer → FP camera is happening
	// (0.8s). The player's actor location is valid and this is the position
	// we use to teleport them back if they wander out of bounds.
	CraterStartPosition = Player->GetActorLocation();

	// ── 2. Re-enable player input ──────────────────────────────────────────
	if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
	{
		FInputModeGameOnly GameMode;
		PC->SetInputMode(GameMode);
		PC->bShowMouseCursor = false;
	}

	UE_LOG(LogTemp, Log,
		TEXT("ATutorialDirector: CS_TutorialIntro finished. "
		     "Input restored. CraterStartPosition = %s"),
		*CraterStartPosition.ToString());

	// ── 3. Notify the Blueprint child class ───────────────────────────────
	BP_OnIntroFinished();
}

// ─────────────────────────────────────────────────────────────────────────────
// PRIVATE: OnOxygenSphereEndOverlap
// Fires when ANY actor leaves the BaseCamp bubble. Filtered to player only.
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::OnOxygenSphereEndOverlap(
	UPrimitiveComponent* OverlappedComp,
	AActor*              OtherActor,
	UPrimitiveComponent* OtherComp,
	int32                OtherBodyIndex)
{
	// Only react to the player character.
	AAlphaExilemetCharacter* Player = Cast<AAlphaExilemetCharacter>(OtherActor);
	if (!Player) return;

	// Guard: if the intro hasn't finished yet (e.g. the level just loaded and
	// the capsule is settling), CraterStartPosition is zero — do nothing.
	if (CraterStartPosition.IsZero()) return;

	// Teleport the player back to the center of the crater.
	// Use SetActorLocation (NOT SetActorTransform with TeleportPhysics) so the
	// Character Movement Component resets cleanly without physics artifacts.
	//Player->SetActorLocation(CraterStartPosition, false, nullptr, ETeleportType::TeleportPhysics);

	UE_LOG(LogTemp, Log,
		TEXT("ATutorialDirector: Player left tutorial boundary. Teleported back to %s."),
		*CraterStartPosition.ToString());

	// Let the Blueprint child add camera-fade FX, UI flash, etc.
	BP_OnPlayerLeftBase();
}

// ─────────────────────────────────────────────────────────────────────────────
// PRIVATE HELPERS
// ─────────────────────────────────────────────────────────────────────────────

AAlphaExilemetCharacter* ATutorialDirector::GetTutorialPlayer() const
{
	return Cast<AAlphaExilemetCharacter>(
		UGameplayStatics::GetPlayerCharacter(this, 0));
}

AActor* ATutorialDirector::FindBaseCamp() const
{
	return UGameplayStatics::GetActorOfClass(GetWorld(), ABaseCamp::StaticClass());
}