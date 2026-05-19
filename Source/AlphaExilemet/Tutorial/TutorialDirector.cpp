#include "TutorialDirector.h"
#include "SkullProp.h"

#include "Kismet/GameplayStatics.h"
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/PlayerCameraManager.h"

#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"

#include "AlphaExilemet/AlphaExilemetCharacter.h"
#include "AlphaExilemet/BaseCamp.h"
#include "AlphaExilemet/Tools/ToolBase.h"
#include "AlphaExilemet/Core/AlphaStreamingSubsystem.h"
#include "Camera/CameraComponent.h"

ATutorialDirector::ATutorialDirector()
{
	// Tick is needed for the smooth CineCamera→Player transition.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false; // only enabled when transition is active
}

void ATutorialDirector::BeginPlay()
{
	Super::BeginPlay();
	BP_RegisterWithGameMode();
}

void ATutorialDirector::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bTransitionActive)
	{
		TickSmoothTransition(DeltaTime);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// InitializeTutorial
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::InitializeTutorial()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// ── 1. CACHE PLAYER ──────────────────────────────────────────────────────
	CachedPlayer = Cast<AAlphaExilemetCharacter>(
		UGameplayStatics::GetPlayerCharacter(this, 0));

	if (!CachedPlayer)
	{
		UE_LOG(LogTemp, Error,
			TEXT("ATutorialDirector::InitializeTutorial — Player pawn not found."));
		return;
	}

	CachedPC = Cast<APlayerController>(CachedPlayer->GetController());
	if (!CachedPC)
	{
		UE_LOG(LogTemp, Error,
			TEXT("ATutorialDirector::InitializeTutorial — PlayerController not found."));
		return;
	}

	// ── 2. WIRE SKULL ────────────────────────────────────────────────────────
	if (SkullRef) SkullRef->SetDirector(this);
	else UE_LOG(LogTemp, Warning,
		TEXT("ATutorialDirector::InitializeTutorial — SkullRef null."));

	// ── 3. HIDE MAIN HUD ─────────────────────────────────────────────────────
	BP_HideHUD();

	// ── 4. DISABLE SURVIVAL ──────────────────────────────────────────────────
	CachedPlayer->bIsSurvivalActive = false;

	// ── 5. VALIDATE SEQUENCE ─────────────────────────────────────────────────
	if (!IntroSequenceRef)
	{
		UE_LOG(LogTemp, Error,
			TEXT("ATutorialDirector::InitializeTutorial — IntroSequenceRef is null."));
		return;
	}

	IntroSequencePlayer = IntroSequenceRef->GetSequencePlayer();
	if (!IntroSequencePlayer)
	{
		UE_LOG(LogTemp, Error, TEXT("ATutorialDirector — IntroSequenceRef has no SequencePlayer."));
		return;
	}

	// ── 6. HIDE REAL PLAYER ──────────────────────────────────────────────────
	HidePlayerForCutscene();

	// ── 7. LOCK ALL INPUT ────────────────────────────────────────────────────
	LockPlayerInputFull();

	// ── 8. BIND CUTSCENE END ─────────────────────────────────────────────────
	IntroSequencePlayer->OnStop.AddUniqueDynamic(
		this, &ATutorialDirector::OnIntroSequenceFinished);

	// ── 9. SNAP TO CINECAM BEFORE PLAY ───────────────────────────────────────
	if (TutorialCineCamRef)
	{
		CachedPC->SetViewTargetWithBlend(TutorialCineCamRef, 0.0f);
	}

	// ── 10. PLAY ─────────────────────────────────────────────────────────────
	IntroSequencePlayer->Play();
	UE_LOG(LogTemp, Log, TEXT("ATutorialDirector: Cutscene playing."));
}

// ─────────────────────────────────────────────────────────────────────────────
// PROXY SWAP HELPERS
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::HidePlayerForCutscene()
{
	if (!CachedPlayer) return;
	CachedPlayer->SetActorHiddenInGame(true);
	if (USkeletalMeshComponent* Mesh = CachedPlayer->GetMesh())
		Mesh->SetVisibility(false, true);

	UE_LOG(LogTemp, Log, TEXT("ATutorialDirector: Real player hidden for cutscene."));
}

AActor* ATutorialDirector::FindCutsceneProxy() const
{
	if (ProxyCharacterTag.IsNone())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("ATutorialDirector::FindCutsceneProxy — ProxyCharacterTag is empty."));
		return nullptr;
	}

	TArray<AActor*> Tagged;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), ProxyCharacterTag, Tagged);

	if (Tagged.Num() == 0)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("ATutorialDirector::FindCutsceneProxy — No actor tagged '%s'."),
			*ProxyCharacterTag.ToString());
		return nullptr;
	}

	return Tagged[0];
}

void ATutorialDirector::ExecuteProxySwap()
{
	if (!CachedPlayer || !CachedPC)
	{
		UE_LOG(LogTemp, Error, TEXT("ATutorialDirector::ExecuteProxySwap — CachedPlayer or CachedPC null."));
		return;
	}

	// Find proxy and teleport the real player to its final position.
	if (AActor* Proxy = FindCutsceneProxy())
	{
		CachedPlayer->SetActorLocationAndRotation(
			Proxy->GetActorLocation(),
			Proxy->GetActorRotation(),
			false, nullptr, ETeleportType::TeleportPhysics);

		CachedPC->SetControlRotation(TransitionTargetRotation);

		// Hide the proxy — Sequencer will destroy it when the sequence ends.
		Proxy->SetActorHiddenInGame(true);

		UE_LOG(LogTemp, Log,
			TEXT("ATutorialDirector::ExecuteProxySwap — Teleported to proxy pos=%s."),
			*Proxy->GetActorLocation().ToString());
	}
	else
	{
		// No proxy — just unhide the player at their spawn location.
		UE_LOG(LogTemp, Warning,
			TEXT("ATutorialDirector::ExecuteProxySwap — No proxy found. Unhiding at spawn."));
	}

	// Unhide the real player.
	CachedPlayer->SetActorHiddenInGame(false);
	if (USkeletalMeshComponent* Mesh = CachedPlayer->GetMesh())
		Mesh->SetVisibility(true, true);

	// Hand camera back to the player — instant (0.0f) because the CineCamera
	// is already AT the player head position after the smooth transition.
	CachedPC->SetViewTargetWithBlend(CachedPlayer, 0.0f);

	UE_LOG(LogTemp, Log, TEXT("ATutorialDirector::ExecuteProxySwap — Swap complete."));
}

// ─────────────────────────────────────────────────────────────────────────────
// SMOOTH CUTSCENE TRANSITION
//
// Called from OnIntroSequenceFinished. Instead of an instant proxy swap:
//   1. We record the CineCamera's current world transform.
//   2. We record where the player's head socket currently is.
//   3. Tick() lerps the CineCamera from (1) to (2) over CutsceneTransitionBlendTime.
//   4. When the CineCamera is within TransitionSnapDistance, ExecuteProxySwap fires.
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::TickSmoothTransition(float DeltaTime)
{
	if (!SequenceEndCameraRef || !CachedPlayer) return;

	TransitionElapsed += DeltaTime;

	// Re-sample the head socket every tick because the player root may drift
	// slightly from physics (it's hidden but physics is still active).
	if (USkeletalMeshComponent* Mesh = CachedPlayer->GetMesh())
	{
		FTransform HeadTransform = Mesh->GetSocketTransform(FName("head"), RTS_World);
		TransitionTargetLocation = HeadTransform.GetLocation();
		TransitionTargetRotation = HeadTransform.GetRotation().Rotator();
	}

	float Alpha = (CutsceneTransitionBlendTime > 0.0f)
		? FMath::Clamp(TransitionElapsed / CutsceneTransitionBlendTime, 0.0f, 1.0f)
		: 1.0f;

	// Smooth step for a more natural deceleration feel.
	float SmoothedAlpha = FMath::SmoothStep(0.0f, 1.0f, Alpha);

	FVector  NewLoc = FMath::Lerp(TransitionStartLocation, TransitionTargetLocation, SmoothedAlpha);
	FRotator NewRot = FMath::Lerp(TransitionStartRotation, TransitionTargetRotation, SmoothedAlpha);

	SequenceEndCameraRef->SetActorLocationAndRotation(NewLoc, NewRot);

	// Check if we're close enough to snap.
	float Dist = FVector::Dist(NewLoc, TransitionTargetLocation);
	if (Dist <= TransitionSnapDistance || Alpha >= 1.0f)
	{
		// Transition complete — stop ticking and do the final swap.
		bTransitionActive = false;
		SetActorTickEnabled(false);

		// Snap the camera exactly to the head socket before swapping.
		SequenceEndCameraRef->SetActorLocationAndRotation(
			TransitionTargetLocation, TransitionTargetRotation);

		ExecuteProxySwap();

		// Now restore movement input and switch to game input mode.
		RestorePlayerMoveInput();

		FInputModeGameOnly GameMode;
		CachedPC->SetInputMode(GameMode);
		CachedPC->bShowMouseCursor = false;

		// Save crater position.
		CraterStartPosition = CachedPlayer->GetActorLocation();

		// Wire boundary guard.
		if (AActor* BaseCampActor = FindBaseCamp())
		{
			if (ABaseCamp* BaseCamp = Cast<ABaseCamp>(BaseCampActor))
			{
				if (BaseCamp->OxygenSphere)
				{
					BaseCamp->OxygenSphere->OnComponentEndOverlap.AddUniqueDynamic(
						this, &ATutorialDirector::OnOxygenSphereEndOverlap);
					UE_LOG(LogTemp, Log, TEXT("ATutorialDirector: OxygenSphere boundary guard active."));
				}
			}
		}

		// Spawn tutorial pickaxe.
		if (TutorialPickaxeClass && !SpawnedTutorialPickaxe)
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			SpawnedTutorialPickaxe = GetWorld()->SpawnActor<AToolBase>(
				TutorialPickaxeClass,
				CachedPlayer->GetActorLocation(),
				CachedPlayer->GetActorRotation(),
				Params);

			if (SpawnedTutorialPickaxe)
			{
				SpawnedTutorialPickaxe->SetActorEnableCollision(false);
				CachedPlayer->AddToolToInventory(SpawnedTutorialPickaxe);
				CachedPlayer->StartWieldTool(0);
			}
		}

		UE_LOG(LogTemp, Log,
			TEXT("ATutorialDirector: Smooth transition complete. CraterPos=%s."),
			*CraterStartPosition.ToString());

		BP_OnIntroFinished();
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// OnIntroSequenceFinished
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::OnIntroSequenceFinished()
{
	if (!CachedPlayer || !CachedPC) return;

	// If no CineCamera or no blend time, fall back to the instant proxy swap.
	if (!SequenceEndCameraRef || CutsceneTransitionBlendTime <= 0.0f)
	{
		ExecuteProxySwap();

		RestorePlayerMoveInput();
		FInputModeGameOnly GameMode;
		CachedPC->SetInputMode(GameMode);
		CachedPC->bShowMouseCursor = false;

		CraterStartPosition = CachedPlayer->GetActorLocation();

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

		if (TutorialPickaxeClass && !SpawnedTutorialPickaxe)
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			SpawnedTutorialPickaxe = GetWorld()->SpawnActor<AToolBase>(
				TutorialPickaxeClass,
				CachedPlayer->GetActorLocation(),
				CachedPlayer->GetActorRotation(),
				Params);

			if (SpawnedTutorialPickaxe)
			{
				SpawnedTutorialPickaxe->SetActorEnableCollision(false);
				CachedPlayer->AddToolToInventory(SpawnedTutorialPickaxe);
				CachedPlayer->StartWieldTool(0);
			}
		}

		BP_OnIntroFinished();
		return;
	}

	// ── SMOOTH TRANSITION PATH ────────────────────────────────────────────────
	// Record the camera's current world transform as the starting point.
	TransitionStartLocation = SequenceEndCameraRef->GetActorLocation();
	TransitionStartRotation = SequenceEndCameraRef->GetActorRotation();

	// Sample the player head socket as the initial target.
	if (USkeletalMeshComponent* Mesh = CachedPlayer->GetMesh())
	{
		FTransform HeadTransform = Mesh->GetSocketTransform(FName("head"), RTS_World);
		TransitionTargetLocation = HeadTransform.GetLocation();
		TransitionTargetRotation = HeadTransform.GetRotation().Rotator();
	}
	else
	{
		// Fallback: use the player camera component's world location.
		if (CachedPlayer->FirstPersonCameraComponent)
		{
			TransitionTargetLocation = CachedPlayer->FirstPersonCameraComponent->GetComponentLocation();
			TransitionTargetRotation = CachedPlayer->FirstPersonCameraComponent->GetComponentRotation();
		}
	}

	TransitionElapsed  = 0.0f;
	bTransitionActive  = true;

	// Keep SequenceEndCameraRef as the view target so the player sees
	// the CineCamera moving smoothly into the head position.
	CachedPC->SetViewTargetWithBlend(SequenceEndCameraRef, 0.0f);

	// Enable tick for the smooth lerp.
	SetActorTickEnabled(true);

	UE_LOG(LogTemp, Log,
		TEXT("ATutorialDirector: Smooth camera transition started. Blend time=%.2fs."),
		CutsceneTransitionBlendTime);
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

// ─────────────────────────────────────────────────────────────────────────────
// Teleport sequence
// ─────────────────────────────────────────────────────────────────────────────

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
	CachedPlayer->SetActorLocation(
		CraterStartPosition, false, nullptr, ETeleportType::TeleportPhysics);
	if (APlayerCameraManager* Cam = CachedPC->PlayerCameraManager)
		Cam->StartCameraFade(1.f, 0.f, 1.0f, FLinearColor::Black, false, false);
	GetWorldTimerManager().SetTimer(TeleportFadeInHandle,
		this, &ATutorialDirector::OnTeleportComplete, 1.0f, false);
}

void ATutorialDirector::OnTeleportComplete()
{
	RestorePlayerMoveInput();
}

// ─────────────────────────────────────────────────────────────────────────────
// ClearTutorialPickaxe
// ─────────────────────────────────────────────────────────────────────────────

void ATutorialDirector::ClearTutorialPickaxe()
{
	if (!CachedPlayer || !SpawnedTutorialPickaxe) return;

	CachedPlayer->OwnedTools.Remove(SpawnedTutorialPickaxe);
	if (CachedPlayer->CurrentTool == SpawnedTutorialPickaxe)
	{
		CachedPlayer->CurrentTool     = nullptr;
		CachedPlayer->ActiveToolIndex = -1;
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
	else UE_LOG(LogTemp, Error, TEXT("ATutorialDirector::OnSkullInteracted — SkullRef null!"));

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
	{
		if (UAlphaStreamingSubsystem* SS = GI->GetSubsystem<UAlphaStreamingSubsystem>())
			SS->HandleTutorialCompletion();
	}
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
	{
		if (UCharacterMovementComponent* Mv = CachedPlayer->GetCharacterMovement())
		{
			if (Mv->MovementMode == MOVE_None)
				Mv->SetMovementMode(MOVE_Walking);
		}
	}
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
	return Cast<AAlphaExilemetCharacter>(
		UGameplayStatics::GetPlayerCharacter(this, 0));
}

AActor* ATutorialDirector::FindBaseCamp() const
{
	return UGameplayStatics::GetActorOfClass(GetWorld(), ABaseCamp::StaticClass());
}
