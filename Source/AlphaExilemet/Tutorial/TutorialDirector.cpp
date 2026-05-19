#include "TutorialDirector.h"
#include "SkullProp.h"

#include "Kismet/GameplayStatics.h"
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"

#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"

#include "AlphaExilemet/AlphaExilemetCharacter.h"
#include "AlphaExilemet/BaseCamp.h"
#include "AlphaExilemet/Tools/ToolBase.h"
#include "AlphaExilemet/Core/AlphaStreamingSubsystem.h"

ATutorialDirector::ATutorialDirector()
{
	PrimaryActorTick.bCanEverTick          = true;
	PrimaryActorTick.bStartWithTickEnabled = false; // enabled only during transition
}

void ATutorialDirector::BeginPlay()
{
	Super::BeginPlay();
	BP_RegisterWithGameMode();
}

void ATutorialDirector::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (bTransitionActive) TickSmoothTransition(DeltaTime);
}

// ─────────────────────────────────────────────────────────────────────────────
// InitializeTutorial
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::InitializeTutorial()
{
	UWorld* World = GetWorld();
	if (!World) return;

	CachedPlayer = Cast<AAlphaExilemetCharacter>(
		UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!CachedPlayer)
	{
		UE_LOG(LogTemp, Error, TEXT("ATutorialDirector::InitializeTutorial — no player pawn."));
		return;
	}

	CachedPC = Cast<APlayerController>(CachedPlayer->GetController());
	if (!CachedPC)
	{
		UE_LOG(LogTemp, Error, TEXT("ATutorialDirector::InitializeTutorial — no PlayerController."));
		return;
	}

	if (SkullRef) SkullRef->SetDirector(this);

	BP_HideHUD();
	CachedPlayer->bIsSurvivalActive = false;

	if (!IntroSequenceRef)
	{
		UE_LOG(LogTemp, Error, TEXT("ATutorialDirector::InitializeTutorial — IntroSequenceRef null."));
		return;
	}

	IntroSequencePlayer = IntroSequenceRef->GetSequencePlayer();
	if (!IntroSequencePlayer)
	{
		UE_LOG(LogTemp, Error, TEXT("ATutorialDirector — no SequencePlayer on IntroSequenceRef."));
		return;
	}

	HidePlayerForCutscene();
	LockPlayerInputFull();

	IntroSequencePlayer->OnStop.AddUniqueDynamic(
		this, &ATutorialDirector::OnIntroSequenceFinished);

	if (TutorialCineCamRef)
		CachedPC->SetViewTargetWithBlend(TutorialCineCamRef, 0.0f);

	IntroSequencePlayer->Play();
	UE_LOG(LogTemp, Log, TEXT("ATutorialDirector: cutscene playing."));
}

// ─────────────────────────────────────────────────────────────────────────────
// PROXY SWAP
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::HidePlayerForCutscene()
{
	if (!CachedPlayer) return;
	CachedPlayer->SetActorHiddenInGame(true);
	if (USkeletalMeshComponent* M = CachedPlayer->GetMesh())
		M->SetVisibility(false, true);
}

AActor* ATutorialDirector::FindCutsceneProxy() const
{
	if (ProxyCharacterTag.IsNone()) return nullptr;
	TArray<AActor*> Tagged;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), ProxyCharacterTag, Tagged);
	return Tagged.Num() > 0 ? Tagged[0] : nullptr;
}

void ATutorialDirector::ExecuteProxySwap()
{
	if (!CachedPlayer || !CachedPC) return;

	if (AActor* Proxy = FindCutsceneProxy())
	{
		CachedPlayer->SetActorLocationAndRotation(
			Proxy->GetActorLocation(), Proxy->GetActorRotation(),
			false, nullptr, ETeleportType::TeleportPhysics);

		// Use the transition target rotation (FirstPersonCamera direction).
		if (CachedPlayer->FirstPersonCameraComponent)
			CachedPC->SetControlRotation(
				CachedPlayer->FirstPersonCameraComponent->GetComponentRotation());

		Proxy->SetActorHiddenInGame(true);
	}

	CachedPlayer->SetActorHiddenInGame(false);
	if (USkeletalMeshComponent* M = CachedPlayer->GetMesh())
		M->SetVisibility(true, true);

	// Instant switch — TempTransitionCamera is already at FirstPersonCamera position.
	CachedPC->SetViewTargetWithBlend(CachedPlayer, 0.0f);

	// Clean up the temporary camera.
	if (TempTransitionCamera)
	{
		TempTransitionCamera->Destroy();
		TempTransitionCamera = nullptr;
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// OnIntroSequenceFinished
//
// Called by the Level Sequence OnStop delegate.
// Reads the current camera transform from PlayerCameraManager — this is the
// exact world position/rotation the Sequencer's CineCamera left it on.
// Spawns a temporary ACameraActor there and starts lerping it toward the
// player's FirstPersonCameraComponent.
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::OnIntroSequenceFinished()
{
	if (!CachedPlayer || !CachedPC) return;

	// ── INSTANT PATH ─────────────────────────────────────────────────────────
	if (CutsceneTransitionBlendTime <= 0.0f)
	{
		ExecuteProxySwap();
		FinishCutsceneHandoff();
		return;
	}

	// ── SMOOTH PATH ──────────────────────────────────────────────────────────
	// Read camera position/rotation from PlayerCameraManager — exactly where
	// the Sequencer left it on the last frame.
	FVector  SequencerCamLoc = FVector::ZeroVector;
	FRotator SequencerCamRot = FRotator::ZeroRotator;

	if (APlayerCameraManager* CamMgr = CachedPC->PlayerCameraManager)
	{
		SequencerCamLoc = CamMgr->GetCameraLocation();
		SequencerCamRot = CamMgr->GetCameraRotation();
	}
	else if (CachedPlayer->FirstPersonCameraComponent)
	{
		// Fallback: start from the player camera itself (instant).
		SequencerCamLoc = CachedPlayer->FirstPersonCameraComponent->GetComponentLocation();
		SequencerCamRot = CachedPlayer->FirstPersonCameraComponent->GetComponentRotation();
	}

	// Spawn a temporary ACameraActor at the Sequencer's last camera position.
	// This becomes our view target during the lerp.
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	TempTransitionCamera = GetWorld()->SpawnActor<ACameraActor>(
		ACameraActor::StaticClass(), SequencerCamLoc, SequencerCamRot, Params);

	if (!TempTransitionCamera)
	{
		UE_LOG(LogTemp, Warning, TEXT("ATutorialDirector: failed to spawn TempTransitionCamera. Falling back to instant."));
		ExecuteProxySwap();
		FinishCutsceneHandoff();
		return;
	}

	// Set the temp camera as the active view target so the player sees it move.
	CachedPC->SetViewTargetWithBlend(TempTransitionCamera, 0.0f);

	// Record start position.
	TransitionStartLocation = SequencerCamLoc;
	TransitionStartRotation = SequencerCamRot;
	TransitionElapsed       = 0.0f;
	bTransitionActive       = true;

	SetActorTickEnabled(true);

	UE_LOG(LogTemp, Log,
		TEXT("ATutorialDirector: smooth transition started from %s. BlendTime=%.2f"),
		*SequencerCamLoc.ToString(), CutsceneTransitionBlendTime);
}

// ─────────────────────────────────────────────────────────────────────────────
// TickSmoothTransition
//
// Moves TempTransitionCamera toward FirstPersonCameraComponent each tick.
// When close enough, fires ExecuteProxySwap and FinishCutsceneHandoff.
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::TickSmoothTransition(float DeltaTime)
{
	if (!TempTransitionCamera || !CachedPlayer) return;

	// Re-read the target every tick — the player might not have been possessed
	// yet on earlier frames and the component could have been re-attached.
	FVector  TargetLoc = TransitionStartLocation;
	FRotator TargetRot = TransitionStartRotation;

	if (CachedPlayer->FirstPersonCameraComponent)
	{
		TargetLoc = CachedPlayer->FirstPersonCameraComponent->GetComponentLocation();
		TargetRot = CachedPlayer->FirstPersonCameraComponent->GetComponentRotation();
	}

	TransitionElapsed += DeltaTime;

	float Alpha = (CutsceneTransitionBlendTime > 0.0f)
		? FMath::Clamp(TransitionElapsed / CutsceneTransitionBlendTime, 0.0f, 1.0f)
		: 1.0f;

	// Smooth step for a natural ease-in / ease-out feel.
	float SA = FMath::SmoothStep(0.0f, 1.0f, Alpha);

	FVector  NewLoc = FMath::Lerp(TransitionStartLocation, TargetLoc, SA);
	FRotator NewRot = FMath::Lerp(TransitionStartRotation, TargetRot, SA);

	TempTransitionCamera->SetActorLocationAndRotation(NewLoc, NewRot);

	if (FVector::Dist(NewLoc, TargetLoc) <= TransitionSnapDistance || Alpha >= 1.0f)
	{
		bTransitionActive = false;
		SetActorTickEnabled(false);

		ExecuteProxySwap();      // unhides player, destroys temp camera, snaps view
		FinishCutsceneHandoff(); // restores input, wires boundary, spawns pickaxe
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// FinishCutsceneHandoff
// Called after proxy swap (instant or smooth). Gives the player full control.
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::FinishCutsceneHandoff()
{
	if (!CachedPlayer || !CachedPC) return;

	RestorePlayerMoveInput();

	FInputModeGameOnly GameMode;
	CachedPC->SetInputMode(GameMode);
	CachedPC->bShowMouseCursor = false;

	// Save crater start position for the boundary guard.
	CraterStartPosition = CachedPlayer->GetActorLocation();

	// Wire OxygenSphere boundary guard.
	if (AActor* BaseCampActor = FindBaseCamp())
	{
		if (ABaseCamp* BC = Cast<ABaseCamp>(BaseCampActor))
		{
			if (BC->OxygenSphere)
			{
				BC->OxygenSphere->OnComponentEndOverlap.AddUniqueDynamic(
					this, &ATutorialDirector::OnOxygenSphereEndOverlap);
				UE_LOG(LogTemp, Log, TEXT("ATutorialDirector: boundary guard active."));
			}
		}
	}

	// Spawn tutorial pickaxe.
	if (TutorialPickaxeClass && !SpawnedTutorialPickaxe)
	{
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnedTutorialPickaxe = GetWorld()->SpawnActor<AToolBase>(
			TutorialPickaxeClass,
			CachedPlayer->GetActorLocation(),
			CachedPlayer->GetActorRotation(), P);

		if (SpawnedTutorialPickaxe)
		{
			SpawnedTutorialPickaxe->SetActorEnableCollision(false);
			CachedPlayer->AddToolToInventory(SpawnedTutorialPickaxe);
			CachedPlayer->StartWieldTool(0);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("ATutorialDirector: handoff complete. CraterPos=%s."),
		*CraterStartPosition.ToString());

	BP_OnIntroFinished();
}

// ─────────────────────────────────────────────────────────────────────────────
// Boundary guard
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::OnOxygenSphereEndOverlap(
	UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, int32)
{
	if (!Cast<AAlphaExilemetCharacter>(OtherActor)) return;
	if (bSkullInteractionActive) return;
	if (CraterStartPosition.IsZero()) return;
	ExecuteTeleportToCrater();
}

void ATutorialDirector::ExecuteTeleportToCrater()
{
	if (!CachedPlayer || !CachedPC || CraterStartPosition.IsZero()) return;
	SuppressPlayerMoveInput();
	if (APlayerCameraManager* Cam = CachedPC->PlayerCameraManager)
		Cam->StartCameraFade(0.f, 1.f, 1.0f, FLinearColor::Black, false, true);
	GetWorldTimerManager().SetTimer(TeleportFadeOutHandle,
		this, &ATutorialDirector::OnTeleportReadyToMove, 1.0f, false);
}

void ATutorialDirector::OnTeleportReadyToMove()
{
	if (!CachedPlayer || !CachedPC) return;
	CachedPlayer->SetActorLocation(CraterStartPosition, false, nullptr, ETeleportType::TeleportPhysics);
	if (APlayerCameraManager* Cam = CachedPC->PlayerCameraManager)
		Cam->StartCameraFade(1.f, 0.f, 1.0f, FLinearColor::Black, false, false);
	GetWorldTimerManager().SetTimer(TeleportFadeInHandle,
		this, &ATutorialDirector::OnTeleportComplete, 1.0f, false);
}

void ATutorialDirector::OnTeleportComplete() { RestorePlayerMoveInput(); }

// ─────────────────────────────────────────────────────────────────────────────
// ClearTutorialPickaxe
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::ClearTutorialPickaxe()
{
	if (!CachedPlayer || !SpawnedTutorialPickaxe) return;

	CachedPlayer->OwnedTools.Remove(SpawnedTutorialPickaxe);
	if (CachedPlayer->CurrentTool == SpawnedTutorialPickaxe)
	{
		CachedPlayer->CurrentTool      = nullptr;
		CachedPlayer->ActiveToolIndex  = -1;
		CachedPlayer->PendingToolIndex = -1;
	}
	SpawnedTutorialPickaxe->Destroy();
	SpawnedTutorialPickaxe = nullptr;

	CachedPlayer->OnInventoryUpdated.Broadcast();
	CachedPlayer->OnToolWielded.Broadcast(-1);
}

// ─────────────────────────────────────────────────────────────────────────────
// Skull interaction sequence
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::OnSkullInteracted()
{
	if (bSkullInteractionActive) return;
	bSkullInteractionActive = true;

	ClearTutorialPickaxe();

	if (SkullRef) SkullRef->StartFlicker();
	BP_PlayMagicSpellSound();

	GetWorldTimerManager().SetTimer(SpellDurationHandle,
		this, &ATutorialDirector::OnSpellDurationComplete, 5.5f, false);
}

void ATutorialDirector::OnSpellDurationComplete()
{
	if (SkullRef) SkullRef->StopAndReset();
	GetWorldTimerManager().SetTimer(SkullPauseHandle,
		this, &ATutorialDirector::OnSkullPausedBeforeBlack, 0.5f, false);
}

void ATutorialDirector::OnSkullPausedBeforeBlack()
{
	LockPlayerInputFull();
	BP_ShowInstantBlack();
	GetWorldTimerManager().SetTimer(PostBlackSoundHandle,
		this, &ATutorialDirector::OnPostBlackDelay, 1.0f, false);
}

void ATutorialDirector::OnPostBlackDelay()
{
	BP_PlayExplosionSequence();
	GetWorldTimerManager().SetTimer(LevelSwapHandle,
		this, &ATutorialDirector::OnLevelSwapReady, 3.0f, false);
}

void ATutorialDirector::OnLevelSwapReady()
{
	if (UGameInstance* GI = GetGameInstance())
		if (UAlphaStreamingSubsystem* SS = GI->GetSubsystem<UAlphaStreamingSubsystem>())
			SS->HandleTutorialCompletion();
}

// ─────────────────────────────────────────────────────────────────────────────
// Input helpers
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::SuppressPlayerMoveInput()
{
	if (CachedPC) CachedPC->SetIgnoreMoveInput(true);
}

void ATutorialDirector::RestorePlayerMoveInput()
{
	if (!CachedPC) return;
	CachedPC->ResetIgnoreMoveInput();
	if (CachedPlayer)
		if (UCharacterMovementComponent* Mv = CachedPlayer->GetCharacterMovement())
			if (Mv->MovementMode == MOVE_None)
				Mv->SetMovementMode(MOVE_Walking);
}

void ATutorialDirector::LockPlayerInputFull()
{
	if (!CachedPlayer || !CachedPC) return;
	if (UCharacterMovementComponent* Mv = CachedPlayer->GetCharacterMovement())
		Mv->DisableMovement();
	CachedPC->SetIgnoreMoveInput(true);
	FInputModeUIOnly UIMode;
	CachedPC->SetInputMode(UIMode);
}

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

AAlphaExilemetCharacter* ATutorialDirector::GetTutorialPlayer() const
{
	return Cast<AAlphaExilemetCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
}

AActor* ATutorialDirector::FindBaseCamp() const
{
	return UGameplayStatics::GetActorOfClass(GetWorld(), ABaseCamp::StaticClass());
}
