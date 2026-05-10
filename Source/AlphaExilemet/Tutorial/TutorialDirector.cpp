#include "TutorialDirector.h"

#include "Kismet/GameplayStatics.h"
#include "Components/SphereComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"

// LevelSequence — use module-relative paths
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"

// Project
#include "AlphaExilemet/AlphaExilemetCharacter.h"
#include "AlphaExilemet/BaseCamp.h"
#include "AlphaExilemet/Tools/ToolBase.h"
#include "AlphaExilemet/Core/AlphaStreamingSubsystem.h"

ATutorialDirector::ATutorialDirector()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ATutorialDirector::BeginPlay()
{
	Super::BeginPlay();
	// Intentionally empty. InitializeTutorial() is called by the GameMode
	// AFTER the player is spawned and possessed — not here.
}

// ─────────────────────────────────────────────────────────────────────────────
// InitializeTutorial
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::InitializeTutorial()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// 1. Player must exist ──────────────────────────────────────────────────
	AAlphaExilemetCharacter* Player = GetTutorialPlayer();
	if (!Player)
	{
		UE_LOG(LogTemp, Error,
			TEXT("ATutorialDirector::InitializeTutorial — Player not found. "
			     "Call this AFTER InitializePlayer() in the GameMode."));
		return;
	}

	// 2. Hide HUD immediately — player just spawned, HUD may briefly appear
	BP_HideHUD();

	// 3. Find Level Sequence by tag ─────────────────────────────────────────
	TArray<AActor*> TaggedActors;
	UGameplayStatics::GetAllActorsWithTag(World, IntroSequenceTag, TaggedActors);

	if (TaggedActors.Num() == 0)
	{
		UE_LOG(LogTemp, Error,
			TEXT("ATutorialDirector::InitializeTutorial — No actor tagged '%s'. "
			     "Select Cutscene1 → Details → Actor → Tags. Case-sensitive."),
			*IntroSequenceTag.ToString());
		return;
	}

	ALevelSequenceActor* SeqActor = Cast<ALevelSequenceActor>(TaggedActors[0]);
	if (!SeqActor)
	{
		UE_LOG(LogTemp, Error,
			TEXT("ATutorialDirector::InitializeTutorial — Tagged actor is not a LevelSequenceActor."));
		return;
	}

	IntroSequencePlayer = SeqActor->GetSequencePlayer();
	if (!IntroSequencePlayer)
	{
		UE_LOG(LogTemp, Error,
			TEXT("ATutorialDirector::InitializeTutorial — LevelSequenceActor has no SequencePlayer. "
			     "Assign a Level Sequence asset to Cutscene1."));
		return;
	}

	// 4. Bind OnStop ────────────────────────────────────────────────────────
	IntroSequencePlayer->OnStop.AddUniqueDynamic(
		this, &ATutorialDirector::OnIntroSequenceFinished);

	// 5. Bind out-of-bounds overlap on BaseCamp ─────────────────────────────
	// Bound here (not BeginPlay) so the callback only fires when the player exists.
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

	// 6. Disable player input before cinematic ─────────────────────────────
	if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
	{
		FInputModeUIOnly UIMode;
		PC->SetInputMode(UIMode);
		PC->bShowMouseCursor = false;
	}

	// 7. Ensure survival is off during Tutorial ────────────────────────────
	Player->bIsSurvivalActive = false;

	// 8. Play the intro sequence ────────────────────────────────────────────
	// REQUIREMENT: Cutscene1 must have a Camera Cut track with CineCameraActor.
	IntroSequencePlayer->Play();

	UE_LOG(LogTemp, Log,
		TEXT("ATutorialDirector: Cutscene started. Input disabled. HUD hidden."));
}

// ─────────────────────────────────────────────────────────────────────────────
// OnIntroSequenceFinished — bound to IntroSequencePlayer::OnStop
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::OnIntroSequenceFinished()
{
	AAlphaExilemetCharacter* Player = GetTutorialPlayer();
	if (!Player) return;

	// 1. Capture crater position — where Finn stands at end of cinematic.
	//    This is the position BP_OnPlayerLeftBase will teleport back to.
	CraterStartPosition = Player->GetActorLocation();

	// 2. Restore input
	if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
	{
		FInputModeGameOnly GameMode;
		PC->SetInputMode(GameMode);
		PC->bShowMouseCursor = false;
	}

	// 3. Spawn and immediately equip the tutorial pickaxe
	if (TutorialPickaxeClass && SpawnedTutorialPickaxe == nullptr)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		SpawnedTutorialPickaxe = GetWorld()->SpawnActor<AToolBase>(
			TutorialPickaxeClass,
			Player->GetActorLocation(),
			Player->GetActorRotation(),
			SpawnParams);

		if (SpawnedTutorialPickaxe)
		{
			SpawnedTutorialPickaxe->SetActorEnableCollision(false);
			Player->AddToolToInventory(SpawnedTutorialPickaxe);
			Player->StartWieldTool(0);
		}
	}

	UE_LOG(LogTemp, Log,
		TEXT("ATutorialDirector: Intro finished. CraterPos=%s. Pickaxe equipped."),
		*CraterStartPosition.ToString());

	// 4. Tell Blueprint to fade the HUD back in and show any hint UI
	BP_OnIntroFinished();
}

// ─────────────────────────────────────────────────────────────────────────────
// OnOxygenSphereEndOverlap
// C++ does NOT teleport — it only validates and calls the BP event.
// The BP event owns the full fade→teleport→unfade sequence.
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::OnOxygenSphereEndOverlap(
	UPrimitiveComponent* OverlappedComp,
	AActor*              OtherActor,
	UPrimitiveComponent* OtherComp,
	int32                OtherBodyIndex)
{
	// Only react to the player
	AAlphaExilemetCharacter* Player = Cast<AAlphaExilemetCharacter>(OtherActor);
	if (!Player) return;

	// Guard: zero until cinematic ends. Prevents firing during level load.
	if (CraterStartPosition.IsZero()) return;

	UE_LOG(LogTemp, Log,
		TEXT("ATutorialDirector: Player left boundary. Calling BP_OnPlayerLeftBase."));

	// Blueprint handles: disable input, fade black, teleport, fade clear, enable input.
	BP_OnPlayerLeftBase();
}

// ─────────────────────────────────────────────────────────────────────────────
// ClearTutorialPickaxe
// Call this before HandleTutorialCompletion fires.
// Removes the pickaxe from inventory and destroys the actor.
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::ClearTutorialPickaxe()
{
	AAlphaExilemetCharacter* Player = GetTutorialPlayer();
	if (!Player) return;

	if (SpawnedTutorialPickaxe)
	{
		// Remove from inventory array
		Player->OwnedTools.Remove(SpawnedTutorialPickaxe);

		// If this tool was actively held, clear the current tool reference
		if (Player->CurrentTool == SpawnedTutorialPickaxe)
		{
			Player->CurrentTool = nullptr;
			Player->ActiveToolIndex = -1;
		}

		// Destroy the actor from the world
		SpawnedTutorialPickaxe->Destroy();
		SpawnedTutorialPickaxe = nullptr;

		// Broadcast so the HUD tool slot updates
		Player->OnInventoryUpdated.Broadcast();
		Player->OnToolWielded.Broadcast(-1);
	}

	UE_LOG(LogTemp, Log, TEXT("ATutorialDirector: Tutorial pickaxe cleared."));
}

// ─────────────────────────────────────────────────────────────────────────────
// OnSkullInteracted — called by BP_SkullProp via DirectorRef
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::OnSkullInteracted()
{
	if (bSkullInteractionActive) return;
	bSkullInteractionActive = true;

	AAlphaExilemetCharacter* Player = GetTutorialPlayer();
	if (!Player) return;

	// Freeze movement and input immediately
	if (UCharacterMovementComponent* Movement = Player->GetCharacterMovement())
	{
		Movement->DisableMovement();
	}
	if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
	{
		FInputModeUIOnly UIMode;
		PC->SetInputMode(UIMode);
	}

	// Clear the pickaxe before the level swap
	ClearTutorialPickaxe();

	// Start skull flicker and spell sound simultaneously
	BP_StartSkullFlicker();
	BP_PlayMagicSpellSound();

	// Timer: wait for the 5-second spell sound to finish
	GetWorldTimerManager().SetTimer(
		SpellDurationHandle,
		this,
		&ATutorialDirector::OnSpellDurationComplete,
		5.0f,
		false);
}

// T+5.0s — spell ends, cut to instant black
void ATutorialDirector::OnSpellDurationComplete()
{
	BP_ShowInstantBlack();

	GetWorldTimerManager().SetTimer(
		PostBlackSoundHandle,
		this,
		&ATutorialDirector::OnPostBlackDelay,
		1.0f,
		false);
}

// T+6.0s — explosion sounds
void ATutorialDirector::OnPostBlackDelay()
{
	BP_PlayExplosionSequence();

	GetWorldTimerManager().SetTimer(
		LevelSwapHandle,
		this,
		&ATutorialDirector::OnLevelSwapReady,
		3.5f,
		false);
}

// T+9.5s — trigger level swap
void ATutorialDirector::OnLevelSwapReady()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UAlphaStreamingSubsystem* Streaming =
			GI->GetSubsystem<UAlphaStreamingSubsystem>())
		{
			Streaming->HandleTutorialCompletion();
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// HELPERS
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
