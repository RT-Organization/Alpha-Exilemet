#include "TutorialDirector.h"

#include "Kismet/GameplayStatics.h"
#include "Components/SphereComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"

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
	// Intentionally empty.
	// InitializeTutorial() is called by GM_SimulatorGamemode AFTER the player
	// is spawned and possessed — not from BeginPlay.
}

// ─────────────────────────────────────────────────────────────────────────────
// InitializeTutorial
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::InitializeTutorial()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// 1. Player must exist
	AAlphaExilemetCharacter* Player = GetTutorialPlayer();
	if (!Player)
	{
		UE_LOG(LogTemp, Error,
			TEXT("ATutorialDirector::InitializeTutorial — Player not found. "
			     "Call this AFTER InitializePlayer() in the GameMode."));
		return;
	}

	// 2. Hide HUD immediately
	BP_HideHUD();

	// 3. Find Level Sequence by tag
	TArray<AActor*> TaggedActors;
	UGameplayStatics::GetAllActorsWithTag(World, IntroSequenceTag, TaggedActors);

	if (TaggedActors.Num() == 0)
	{
		UE_LOG(LogTemp, Error,
			TEXT("ATutorialDirector::InitializeTutorial — No actor tagged '%s'. "
			     "Select Cutscene1 -> Details -> Actor -> Tags. Case-sensitive."),
			*IntroSequenceTag.ToString());
		return;
	}

	ALevelSequenceActor* SeqActor = Cast<ALevelSequenceActor>(TaggedActors[0]);
	if (!SeqActor)
	{
		UE_LOG(LogTemp, Error,
			TEXT("ATutorialDirector — Tagged actor is not a LevelSequenceActor."));
		return;
	}

	IntroSequencePlayer = SeqActor->GetSequencePlayer();
	if (!IntroSequencePlayer)
	{
		UE_LOG(LogTemp, Error,
			TEXT("ATutorialDirector — LevelSequenceActor has no Sequence Player. "
			     "Assign a Level Sequence asset to Cutscene1."));
		return;
	}

	// 4. Bind OnStop
	IntroSequencePlayer->OnStop.AddUniqueDynamic(
		this, &ATutorialDirector::OnIntroSequenceFinished);

	// NOTE: OxygenSphere overlap is bound in OnIntroSequenceFinished, NOT here.
	// Reason: BaseCamp may not have fully initialized BeginPlay yet when this
	// function fires. Moving the bind to OnIntroSequenceFinished guarantees
	// BaseCamp exists and CraterStartPosition is set before the overlap can fire.
	// This fixes the BP_OnPlayerLeftBase "never fires" bug.

	// 5. Disable player input
	APlayerController* PC = Cast<APlayerController>(Player->GetController());
	if (PC)
	{
		FInputModeUIOnly UIMode;
		PC->SetInputMode(UIMode);
		PC->bShowMouseCursor = false;
	}

	// 6. Ensure survival is off during Tutorial
	Player->bIsSurvivalActive = false;

	// 7. FIX BUG 1 — Force camera to CineCameraActor INSTANTLY before Play().
	//    This eliminates the single-frame flash of the player FP view that
	//    occurs between possession and Sequencer taking the camera.
	//    Tag the CineCameraActor in L_Tutorial with "TutorialCineCam".
	TArray<AActor*> CamActors;
	UGameplayStatics::GetAllActorsWithTag(World, FName("TutorialCineCam"), CamActors);
	if (CamActors.Num() > 0 && PC)
	{
		// Blend time 0 = instant. No visual transition — the camera just snaps.
		// Sequencer then immediately owns it from frame 0.
		PC->SetViewTargetWithBlend(CamActors[0], 0.0f);
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("ATutorialDirector — No actor tagged 'TutorialCineCam'. "
			     "The 1-frame FP flash may be visible. "
			     "Tag the CineCameraActor in L_Tutorial with 'TutorialCineCam'."));
	}

	// 8. Play the sequence
	IntroSequencePlayer->Play();

	UE_LOG(LogTemp, Log,
		TEXT("ATutorialDirector: Cutscene started. Camera forced to CineCam. Input disabled. HUD hidden."));
}

// ─────────────────────────────────────────────────────────────────────────────
// OnIntroSequenceFinished
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::OnIntroSequenceFinished()
{
	AAlphaExilemetCharacter* Player = GetTutorialPlayer();
	if (!Player) return;

	// Return camera to player immediately
	if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
	{
		PC->SetViewTargetWithBlend(Player, 0.0f);
	}

	// 1. Capture crater position
	CraterStartPosition = Player->GetActorLocation();

	// 2. Restore input
	if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
	{
		FInputModeGameOnly GameMode;
		PC->SetInputMode(GameMode);
		PC->bShowMouseCursor = false;
	}

	// 3. FIX BUG 3 — Bind OxygenSphere overlap HERE, not in InitializeTutorial.
	//    BaseCamp is guaranteed to exist now (level fully loaded + BeginPlay done).
	//    CraterStartPosition is set above, so the zero-check guard in the
	//    overlap callback will pass correctly.
	if (AActor* BaseCampActor = FindBaseCamp())
	{
		if (ABaseCamp* BaseCamp = Cast<ABaseCamp>(BaseCampActor))
		{
			if (BaseCamp->OxygenSphere)
			{
				BaseCamp->OxygenSphere->OnComponentEndOverlap.AddUniqueDynamic(
					this, &ATutorialDirector::OnOxygenSphereEndOverlap);
				UE_LOG(LogTemp, Log,
					TEXT("ATutorialDirector: OxygenSphere EndOverlap bound. Boundary guard active."));
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("ATutorialDirector::OnIntroSequenceFinished — BaseCamp not found. "
			     "Boundary guard inactive."));
	}

	// 4. Spawn and equip the tutorial pickaxe
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
			UE_LOG(LogTemp, Log, TEXT("ATutorialDirector: Tutorial pickaxe spawned and equipped."));
		}
	}
	else if (!TutorialPickaxeClass)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("ATutorialDirector — TutorialPickaxeClass is null. "
			     "Open BP_TutorialDirector -> Class Defaults -> TutorialPickaxeClass -> assign BP_Pickaxe."));
	}

	UE_LOG(LogTemp, Log,
		TEXT("ATutorialDirector: Intro finished. CraterPos=%s."),
		*CraterStartPosition.ToString());

	// 5. Tell Blueprint: show HUD hint (NOT the HUD itself — HUD stays hidden during tutorial)
	BP_OnIntroFinished();
}

// ─────────────────────────────────────────────────────────────────────────────
// OnOxygenSphereEndOverlap
// C++ fires the BP event only. BP owns: disable input, fade, teleport, unfade, enable input.
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::OnOxygenSphereEndOverlap(
	UPrimitiveComponent* OverlappedComp,
	AActor*              OtherActor,
	UPrimitiveComponent* OtherComp,
	int32                OtherBodyIndex)
{
	AAlphaExilemetCharacter* Player = Cast<AAlphaExilemetCharacter>(OtherActor);
	if (!Player) return;

	// CraterStartPosition is zero until cutscene ends — extra safety guard.
	if (CraterStartPosition.IsZero())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("ATutorialDirector::OnOxygenSphereEndOverlap — CraterStartPosition is zero. "
			     "Boundary teleport skipped. This should not happen after the cutscene."));
		return;
	}

	UE_LOG(LogTemp, Log,
		TEXT("ATutorialDirector: Player left boundary. Calling BP_OnPlayerLeftBase."));

	// Blueprint handles the full sequence:
	// SetIgnoreMoveInput(true) -> CameraFade(0->1, 1s) -> Delay(1s)
	// -> SetActorLocation(CraterStartPosition) -> CameraFade(1->0, 1s)
	// -> Delay(1s) -> SetIgnoreMoveInput(false)
	BP_OnPlayerLeftBase();
}

// ─────────────────────────────────────────────────────────────────────────────
// ClearTutorialPickaxe
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::ClearTutorialPickaxe()
{
	AAlphaExilemetCharacter* Player = GetTutorialPlayer();
	if (!Player) return;

	if (SpawnedTutorialPickaxe)
	{
		Player->OwnedTools.Remove(SpawnedTutorialPickaxe);

		if (Player->CurrentTool == SpawnedTutorialPickaxe)
		{
			Player->CurrentTool    = nullptr;
			Player->ActiveToolIndex = -1;
		}

		SpawnedTutorialPickaxe->Destroy();
		SpawnedTutorialPickaxe = nullptr;

		Player->OnInventoryUpdated.Broadcast();
		Player->OnToolWielded.Broadcast(-1);

		UE_LOG(LogTemp, Log, TEXT("ATutorialDirector: Tutorial pickaxe cleared from inventory."));
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// OnSkullInteracted
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::OnSkullInteracted()
{
	if (bSkullInteractionActive) return;
	bSkullInteractionActive = true;

	AAlphaExilemetCharacter* Player = GetTutorialPlayer();
	if (!Player) return;

	if (UCharacterMovementComponent* Movement = Player->GetCharacterMovement())
		Movement->DisableMovement();

	if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
	{
		FInputModeUIOnly UIMode;
		PC->SetInputMode(UIMode);
	}

	ClearTutorialPickaxe();

	BP_StartSkullFlicker();
	BP_PlayMagicSpellSound();

	GetWorldTimerManager().SetTimer(
		SpellDurationHandle,
		this,
		&ATutorialDirector::OnSpellDurationComplete,
		5.0f, false);
}

void ATutorialDirector::OnSpellDurationComplete()
{
	BP_ShowInstantBlack();
	GetWorldTimerManager().SetTimer(
		PostBlackSoundHandle,
		this,
		&ATutorialDirector::OnPostBlackDelay,
		1.0f, false);
}

void ATutorialDirector::OnPostBlackDelay()
{
	BP_PlayExplosionSequence();
	GetWorldTimerManager().SetTimer(
		LevelSwapHandle,
		this,
		&ATutorialDirector::OnLevelSwapReady,
		3.5f, false);
}

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
