#include "WakeUpDirector.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"

#include "AlphaExilemet/AlphaExilemetCharacter.h"
#include "AlphaExilemet/Core/AlphaStreamingSubsystem.h"
#include "Camera/CameraComponent.h"

AWakeUpDirector::AWakeUpDirector()
{
	// Tick needed for the smooth CineCamera→Player transition.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false; // only enabled during transition
}

void AWakeUpDirector::BeginPlay()
{
	Super::BeginPlay();
	BP_RegisterWithGameMode();
}

void AWakeUpDirector::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bTransitionActive)
	{
		TickSmoothTransition(DeltaTime);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// InitializeWakeUp
// ─────────────────────────────────────────────────────────────────────────────

void AWakeUpDirector::InitializeWakeUp()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// ── 1. CACHE PLAYER ──────────────────────────────────────────────────────
	CachedPlayer = Cast<AAlphaExilemetCharacter>(
		UGameplayStatics::GetPlayerCharacter(this, 0));

	if (!CachedPlayer)
	{
		UE_LOG(LogTemp, Error,
			TEXT("AWakeUpDirector::InitializeWakeUp — Player pawn not found."));

		// Safety: broadcast ready so the blackout widget doesn't get stuck.
		if (UGameInstance* GI = GetGameInstance())
			if (UAlphaStreamingSubsystem* SS = GI->GetSubsystem<UAlphaStreamingSubsystem>())
				SS->OnTutorialPlayerReady.Broadcast();
		return;
	}

	CachedPC = Cast<APlayerController>(CachedPlayer->GetController());
	if (!CachedPC)
	{
		UE_LOG(LogTemp, Error,
			TEXT("AWakeUpDirector::InitializeWakeUp — PlayerController not found."));
		return;
	}

	// ── 2. VALIDATE SEQUENCE ─────────────────────────────────────────────────
	if (!WakeUpSequenceRef)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("AWakeUpDirector::InitializeWakeUp — WakeUpSequenceRef null. Enabling directly."));

		CachedPlayer->bIsSurvivalActive = true;
		FInputModeGameOnly GameMode;
		CachedPC->SetInputMode(GameMode);

		if (UGameInstance* GI = GetGameInstance())
			if (UAlphaStreamingSubsystem* SS = GI->GetSubsystem<UAlphaStreamingSubsystem>())
				SS->OnTutorialPlayerReady.Broadcast();

		BP_OnWakeUpComplete();
		return;
	}

	WakeUpSequencePlayer = WakeUpSequenceRef->GetSequencePlayer();
	if (!WakeUpSequencePlayer)
	{
		UE_LOG(LogTemp, Error, TEXT("AWakeUpDirector — WakeUpSequenceRef has no SequencePlayer."));
		return;
	}

	// ── 3. LOCK INPUT ─────────────────────────────────────────────────────────
	{
		FInputModeUIOnly UIMode;
		CachedPC->SetInputMode(UIMode);
		CachedPC->bShowMouseCursor = false;
	}

	if (UCharacterMovementComponent* Mv = CachedPlayer->GetCharacterMovement())
	{
		if (Mv->MovementMode != MOVE_None)
			Mv->DisableMovement();
	}

	// ── 4. OPTIONAL CINECAM ──────────────────────────────────────────────────
	if (WakeUpCineCamRef)
	{
		CachedPC->SetViewTargetWithBlend(WakeUpCineCamRef, 0.0f);
	}

	// ── 5. BIND + PLAY ───────────────────────────────────────────────────────
	WakeUpSequencePlayer->OnStop.AddUniqueDynamic(
		this, &AWakeUpDirector::OnWakeUpSequenceFinished);

	WakeUpSequencePlayer->Play();

	UE_LOG(LogTemp, Log, TEXT("AWakeUpDirector: Wake-up cutscene playing."));
}

// ─────────────────────────────────────────────────────────────────────────────
// OnWakeUpSequenceFinished
// ─────────────────────────────────────────────────────────────────────────────

void AWakeUpDirector::OnWakeUpSequenceFinished()
{
	if (!CachedPlayer || !CachedPC) return;

	// If no CineCamera ref or no blend time, fall back to instant hand-back.
	if (!SequenceEndCameraRef || WakeUpTransitionBlendTime <= 0.0f)
	{
		OnTransitionComplete();
		return;
	}

	// ── SMOOTH TRANSITION PATH ────────────────────────────────────────────────
	// Record where the CineCamera is now as the starting point.
	TransitionStartLocation = SequenceEndCameraRef->GetActorLocation();
	TransitionStartRotation = SequenceEndCameraRef->GetActorRotation();

	// Sample player head socket as target.
	if (USkeletalMeshComponent* Mesh = CachedPlayer->GetMesh())
	{
		FTransform HeadTransform = Mesh->GetSocketTransform(FName("head"), RTS_World);
		TransitionTargetLocation = HeadTransform.GetLocation();
		TransitionTargetRotation = HeadTransform.GetRotation().Rotator();
	}
	else if (CachedPlayer->FirstPersonCameraComponent)
	{
		TransitionTargetLocation = CachedPlayer->FirstPersonCameraComponent->GetComponentLocation();
		TransitionTargetRotation = CachedPlayer->FirstPersonCameraComponent->GetComponentRotation();
	}

	TransitionElapsed = 0.0f;
	bTransitionActive = true;

	// Keep the CineCamera as view target so the player sees the lerp.
	CachedPC->SetViewTargetWithBlend(SequenceEndCameraRef, 0.0f);

	// Enable tick for the smooth lerp.
	SetActorTickEnabled(true);

	UE_LOG(LogTemp, Log,
		TEXT("AWakeUpDirector: Smooth wake-up transition started. Blend time=%.2fs."),
		WakeUpTransitionBlendTime);
}

// ─────────────────────────────────────────────────────────────────────────────
// SMOOTH TRANSITION TICK
// ─────────────────────────────────────────────────────────────────────────────

void AWakeUpDirector::TickSmoothTransition(float DeltaTime)
{
	if (!SequenceEndCameraRef || !CachedPlayer) return;

	TransitionElapsed += DeltaTime;

	// Resample head socket every tick.
	if (USkeletalMeshComponent* Mesh = CachedPlayer->GetMesh())
	{
		FTransform HeadTransform = Mesh->GetSocketTransform(FName("head"), RTS_World);
		TransitionTargetLocation = HeadTransform.GetLocation();
		TransitionTargetRotation = HeadTransform.GetRotation().Rotator();
	}

	float Alpha = (WakeUpTransitionBlendTime > 0.0f)
		? FMath::Clamp(TransitionElapsed / WakeUpTransitionBlendTime, 0.0f, 1.0f)
		: 1.0f;

	float SmoothedAlpha = FMath::SmoothStep(0.0f, 1.0f, Alpha);

	FVector  NewLoc = FMath::Lerp(TransitionStartLocation, TransitionTargetLocation, SmoothedAlpha);
	FRotator NewRot = FMath::Lerp(TransitionStartRotation, TransitionTargetRotation, SmoothedAlpha);

	SequenceEndCameraRef->SetActorLocationAndRotation(NewLoc, NewRot);

	float Dist = FVector::Dist(NewLoc, TransitionTargetLocation);
	if (Dist <= TransitionSnapDistance || Alpha >= 1.0f)
	{
		bTransitionActive = false;
		SetActorTickEnabled(false);

		// Snap exactly to the head socket.
		SequenceEndCameraRef->SetActorLocationAndRotation(
			TransitionTargetLocation, TransitionTargetRotation);

		OnTransitionComplete();
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// OnTransitionComplete — fires after smooth lerp OR immediately on instant swap
// ─────────────────────────────────────────────────────────────────────────────

void AWakeUpDirector::OnTransitionComplete()
{
	if (!CachedPlayer || !CachedPC) return;

	// ── 1. RETURN CAMERA TO PLAYER ───────────────────────────────────────────
	// Instant snap — the CineCamera is already at the head socket position.
	CachedPC->SetViewTargetWithBlend(CachedPlayer, 0.0f);

	// ── 2. RESTORE INPUT ─────────────────────────────────────────────────────
	FInputModeGameOnly GameMode;
	CachedPC->SetInputMode(GameMode);
	CachedPC->bShowMouseCursor = false;
	CachedPC->ResetIgnoreMoveInput();

	// ── 3. RE-ENABLE MOVEMENT ────────────────────────────────────────────────
	if (UCharacterMovementComponent* Mv = CachedPlayer->GetCharacterMovement())
	{
		Mv->SetMovementMode(MOVE_Walking);
	}

	// ── 4. ENABLE SURVIVAL ───────────────────────────────────────────────────
	CachedPlayer->bIsSurvivalActive = true;

	// ── 5. SIGNAL BLACKOUT WIDGET ────────────────────────────────────────────
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UAlphaStreamingSubsystem* SS = GI->GetSubsystem<UAlphaStreamingSubsystem>())
		{
			SS->OnTutorialPlayerReady.Broadcast();
		}
	}

	UE_LOG(LogTemp, Log, TEXT("AWakeUpDirector: Wake-up complete. Player has full control."));

	// ── 6. OPTIONAL BP POLISH ────────────────────────────────────────────────
	BP_OnWakeUpComplete();
}
