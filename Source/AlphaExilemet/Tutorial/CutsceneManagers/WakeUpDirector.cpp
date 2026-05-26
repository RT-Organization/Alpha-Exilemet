#include "WakeUpDirector.h"
#include "AlphaExilemet/Tutorial/CutsceneHelpers/CinematicHandoffComponent.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraActor.h"

#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"

#include "AlphaExilemet/AlphaExilemetCharacter.h"
#include "AlphaExilemet/Core/AlphaStreamingSubsystem.h"

AWakeUpDirector::AWakeUpDirector()
{
	PrimaryActorTick.bCanEverTick = false;

	CinematicHandoff = CreateDefaultSubobject<UCinematicHandoffComponent>(
		TEXT("CinematicHandoff"));
}

void AWakeUpDirector::BeginPlay()
{
	Super::BeginPlay();
	BP_RegisterWithGameMode();
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
			TEXT("AWakeUpDirector::InitializeWakeUp — Player not found. Signalling ready."));

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
			TEXT("AWakeUpDirector::InitializeWakeUp — WakeUpSequenceRef null. Enabling player directly."));

		CachedPlayer->bIsSurvivalActive = true;
		CachedPC->SetInputMode(FInputModeGameOnly());

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

	// ── 3. HIDE PLAYER FOR CUTSCENE ──────────────────────────────────────────
	CachedPlayer->SetActorHiddenInGame(true);
	if (USkeletalMeshComponent* Mesh = CachedPlayer->GetMesh())
		Mesh->SetVisibility(false, true);

	// ── 4. LOCK INPUT ────────────────────────────────────────────────────────
	{
		FInputModeUIOnly UIMode;
		CachedPC->SetInputMode(UIMode);
		CachedPC->bShowMouseCursor = false;
	}

	if (UCharacterMovementComponent* Mv = CachedPlayer->GetCharacterMovement())
		if (Mv->MovementMode != MOVE_None)
			Mv->DisableMovement();

	// ── 5. OPTIONAL PRE-SEQUENCE CINECAM ─────────────────────────────────────
	if (WakeUpCineCamRef)
		CachedPC->SetViewTargetWithBlend(WakeUpCineCamRef, 0.0f);

	// ── 6. BIND + PLAY ───────────────────────────────────────────────────────
	WakeUpSequencePlayer->OnStop.AddUniqueDynamic(
		this, &AWakeUpDirector::OnWakeUpSequenceFinished);

	WakeUpSequencePlayer->Play();

	// Dismiss WB_TutorialBlackout the moment the WakeUp sequence starts.
	// The sequence owns the visuals from here — the blackout is no longer needed.
	if (UGameInstance* GI = GetGameInstance())
		if (UAlphaStreamingSubsystem* SS = GI->GetSubsystem<UAlphaStreamingSubsystem>())
			SS->OnTutorialPlayerReady.Broadcast();

	UE_LOG(LogTemp, Log, TEXT("AWakeUpDirector: Wake-up cutscene playing."));
}

// ─────────────────────────────────────────────────────────────────────────────
// OnWakeUpSequenceFinished
// Same bone-based logic as TutorialDirector::OnIntroSequenceFinished.
// ─────────────────────────────────────────────────────────────────────────────

void AWakeUpDirector::OnWakeUpSequenceFinished()
{
	if (!CachedPlayer || !CachedPC) return;

	// ── A. GHOST CAMERA START ─────────────────────────────────────────────────
	FVector  GhostStartLocation;
	FRotator GhostStartRotation;

	if (LastWakeUpCineCamera)
	{
		GhostStartLocation = LastWakeUpCineCamera->GetActorLocation();
		GhostStartRotation = LastWakeUpCineCamera->GetActorRotation();
	}
	else if (AActor* ViewTarget = CachedPC->GetViewTarget())
	{
		GhostStartLocation = ViewTarget->GetActorLocation();
		GhostStartRotation = ViewTarget->GetActorRotation();
		UE_LOG(LogTemp, Log,
			TEXT("AWakeUpDirector: Ghost start auto-detected from ViewTarget '%s'."),
			*ViewTarget->GetName());
	}
	else
	{
		GhostStartLocation = CachedPlayer->GetActorLocation();
		GhostStartRotation = CachedPlayer->GetActorRotation();
		UE_LOG(LogTemp, Warning,
			TEXT("AWakeUpDirector: No CineCamera found. Assign LastWakeUpCineCamera."));
	}

	// ── B. FIND SK PROXY BY TAG ───────────────────────────────────────────────
	USkeletalMeshComponent* ProxyMesh  = nullptr;
	AActor*                 ProxyActor = nullptr;

	if (!ProxySkeletonTag.IsNone())
	{
		TArray<AActor*> Tagged;
		UGameplayStatics::GetAllActorsWithTag(GetWorld(), ProxySkeletonTag, Tagged);

		if (Tagged.Num() > 0)
		{
			ProxyActor = Tagged[0];
			ProxyMesh  = ProxyActor->FindComponentByClass<USkeletalMeshComponent>();
			UE_LOG(LogTemp, Log,
				TEXT("AWakeUpDirector: Found proxy SK '%s'."), *ProxyActor->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Warning,
				TEXT("AWakeUpDirector: No actor tagged '%s'. "
				     "Set the SK_Manny Spawnable Actor Tag and When Finished = Keep State."),
				*ProxySkeletonTag.ToString());
		}
	}

	// ── C. CAMERA TRAVEL TARGET (head bone) ──────────────────────────────────
	FVector CameraTarget;

	if (ProxyMesh && ProxyMesh->DoesSocketExist(HeadBoneName))
	{
		CameraTarget = ProxyMesh->GetBoneLocation(HeadBoneName);
	}
	else if (ProxyActor)
	{
		CameraTarget = ProxyActor->GetActorLocation()
		             + FVector(0.f, 0.f, CachedPlayer->BaseEyeHeight);
		UE_LOG(LogTemp, Warning,
			TEXT("AWakeUpDirector: Head bone '%s' not found. Using actor loc + EyeHeight."),
			*HeadBoneName.ToString());
	}
	else
	{
		CameraTarget = GhostStartLocation;
	}

	// ── D. PLAYER SPAWN TRANSFORM (root bone) ────────────────────────────────
	FTransform PlayerSpawnTransform;

	if (ProxyMesh && ProxyMesh->DoesSocketExist(RootBoneName))
	{
		FVector RootLoc  = ProxyMesh->GetBoneLocation(RootBoneName);
		FRotator SpawnRot(0.f, ProxyActor->GetActorRotation().Yaw, 0.f);
		PlayerSpawnTransform = FTransform(SpawnRot, RootLoc, FVector::OneVector);
	}
	else if (ProxyActor)
	{
		FRotator SpawnRot(0.f, ProxyActor->GetActorRotation().Yaw, 0.f);
		PlayerSpawnTransform = FTransform(SpawnRot, ProxyActor->GetActorLocation());
	}
	else
	{
		PlayerSpawnTransform = CachedPlayer->GetActorTransform();
		UE_LOG(LogTemp, Warning,
			TEXT("AWakeUpDirector: No proxy. Player will appear at PlayerStart."));
	}

	// ── E. BIND HANDOFF CALLBACK ──────────────────────────────────────────────
	CinematicHandoff->OnHandoffComplete.AddUniqueDynamic(
		this, &AWakeUpDirector::OnWakeUpHandoffComplete);

	// ── F. BEGIN HANDOFF ──────────────────────────────────────────────────────
	CinematicHandoff->BeginHandoff(
		CachedPlayer,
		CachedPC,
		GhostStartLocation,
		GhostStartRotation,
		CameraTarget,
		PlayerSpawnTransform,
		ProxyActor);
}

// ─────────────────────────────────────────────────────────────────────────────
// OnWakeUpHandoffComplete
// ─────────────────────────────────────────────────────────────────────────────

void AWakeUpDirector::OnWakeUpHandoffComplete()
{
	if (!CachedPlayer || !CachedPC) return;

	// Restore input.
	FInputModeGameOnly GameMode;
	CachedPC->SetInputMode(GameMode);
	CachedPC->bShowMouseCursor  = false;
	CachedPC->ResetIgnoreMoveInput();

	// Re-enable movement.
	if (UCharacterMovementComponent* Mv = CachedPlayer->GetCharacterMovement())
		Mv->SetMovementMode(MOVE_Walking);

	// Enable survival — oxygen starts draining from this moment.
	CachedPlayer->bIsSurvivalActive = true;

	UE_LOG(LogTemp, Log, TEXT("AWakeUpDirector: Handoff complete. Player has full control. Survival ON."));

	// Show HUD, play ambience, trigger ship terminal warning, etc.
	BP_OnWakeUpComplete();
}
