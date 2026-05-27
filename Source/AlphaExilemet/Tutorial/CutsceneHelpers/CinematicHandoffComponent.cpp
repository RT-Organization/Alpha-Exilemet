#include "CinematicHandoffComponent.h"

#include "AlphaExilemet/AlphaExilemetCharacter.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

UCinematicHandoffComponent::UCinematicHandoffComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

// ─────────────────────────────────────────────────────────────────────────────
// BeginHandoff
// ─────────────────────────────────────────────────────────────────────────────

void UCinematicHandoffComponent::BeginHandoff(
	AAlphaExilemetCharacter* Player,
	APlayerController*       PC,
	FVector                  GhostStartLocation,
	FRotator                 GhostStartRotation,
	FVector                  CameraTargetLocation,
	FTransform               PlayerSpawnTransform,
	AActor*                  ProxyActorToDestroy)
{
	if (bHandoffInProgress)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("UCinematicHandoffComponent::BeginHandoff — already in progress. Ignoring."));
		return;
	}

	if (!Player || !PC)
	{
		UE_LOG(LogTemp, Error,
			TEXT("UCinematicHandoffComponent::BeginHandoff — Player or PC null. "
			     "Firing OnHandoffComplete as immediate fallback."));
		OnHandoffComplete.Broadcast();
		return;
	}

	UWorld* World = GetWorld();
	if (!World) return;

	bHandoffInProgress      = true;
	CachedPlayer            = Player;
	CachedPC                = PC;
	PendingPlayerSpawn      = PlayerSpawnTransform;
	PendingProxyToDestroy   = ProxyActorToDestroy;
	TravelElapsed           = 0.0f;

	// ── TRAVEL PARAMETERS ─────────────────────────────────────────────────────
	TravelStartPos  = GhostStartLocation;
	TravelEndPos    = CameraTargetLocation;
	TravelStartQuat = GhostStartRotation.Quaternion();

	// End rotation: look toward the player's intended facing direction, pitch=0.
	FRotator EndRot(0.f, PlayerSpawnTransform.GetRotation().Rotator().Yaw, 0.f);
	TravelEndQuat = EndRot.Quaternion();

	// ── CLEAR SEQUENCER'S CINEMATIC BLACK BARS ─────────────────────────────────
	// When the Level Sequence ends its CineCamera track, the player's FP camera
	// still has bConstrainAspectRatio inherited from the CineCamera's post-process
	// settings. This is what causes the black bars after the handoff.
	// Clear it now before we switch view targets.
	if (UCameraComponent* FPCam = Player->FindComponentByClass<UCameraComponent>())
	{
		FPCam->bConstrainAspectRatio = false;
		FPCam->PostProcessSettings.bOverride_VignetteIntensity = false;
	}

	// ── SPAWN GHOST CAMERA AT LAST CINECAMERA POSITION ────────────────────────
	// The ghost sits at the exact same world position + rotation as the sequence's
	// last frame. Snapping the PC view here (zero blend) is INVISIBLE because
	// the rendered image is identical to what the sequence was showing.
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Name = TEXT("CinematicHandoff_Ghost");

	GhostCamera = World->SpawnActor<ACameraActor>(
		ACameraActor::StaticClass(),
		FTransform(GhostStartRotation, GhostStartLocation),
		SpawnParams);

	if (!GhostCamera)
	{
		UE_LOG(LogTemp, Error,
			TEXT("UCinematicHandoffComponent::BeginHandoff — Ghost spawn failed. "
			     "Firing OnHandoffComplete as fallback."));
		bHandoffInProgress = false;
		OnHandoffComplete.Broadcast();
		return;
	}

	// Make sure the ghost camera has no black bars either.
	if (UCameraComponent* GhostCam = GhostCamera->FindComponentByClass<UCameraComponent>())
	{
		GhostCam->bConstrainAspectRatio = false;
	}

	// Snap PC view to ghost — image on screen does not change at all.
	PC->SetViewTargetWithBlend(GhostCamera, 0.0f);

	// ── SAFETY TIMER ──────────────────────────────────────────────────────────
	// If the handoff never completes (race condition, no proxy, etc.) this fires
	// after SafetyInputRestoreDelay seconds and forces the player to be playable.
	World->GetTimerManager().SetTimer(
		SafetyHandle,
		this,
		&UCinematicHandoffComponent::SafetyRestoreInput,
		SafetyInputRestoreDelay,
		false);

	// ── START TRAVELLING ──────────────────────────────────────────────────────
	SetComponentTickEnabled(true);

	UE_LOG(LogTemp, Log,
		TEXT("UCinematicHandoffComponent: Ghost at %s. Travelling %.2fs → %s (head bone)."),
		*GhostStartLocation.ToString(), CameraBlendTime, *CameraTargetLocation.ToString());
}

// ─────────────────────────────────────────────────────────────────────────────
// TickComponent — smooth ghost travel every frame
// ─────────────────────────────────────────────────────────────────────────────

void UCinematicHandoffComponent::TickComponent(
	float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bHandoffInProgress || !GhostCamera) return;

	TravelElapsed = FMath::Min(TravelElapsed + DeltaTime, CameraBlendTime);

	const float Alpha  = (CameraBlendTime > KINDA_SMALL_NUMBER)
	                   ? (TravelElapsed / CameraBlendTime) : 1.f;
	const float Smooth = FMath::SmoothStep(0.f, 1.f, Alpha); // ease-in / ease-out

	GhostCamera->SetActorLocationAndRotation(
		FMath::Lerp(TravelStartPos, TravelEndPos, Smooth),
		FQuat::Slerp(TravelStartQuat, TravelEndQuat, Smooth).Rotator(),
		false, nullptr, ETeleportType::TeleportPhysics);

	if (Alpha >= 1.f)
	{
		SetComponentTickEnabled(false);
		OnArrivalAtTarget();
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// OnArrivalAtTarget
// Ghost reached the head bone. Place player, unhide, switch view.
// ─────────────────────────────────────────────────────────────────────────────

void UCinematicHandoffComponent::OnArrivalAtTarget()
{
	// Cancel the safety timer — we made it in time.
	if (UWorld* World = GetWorld())
		World->GetTimerManager().ClearTimer(SafetyHandle);

	if (!CachedPlayer || !CachedPC)
	{
		if (GhostCamera) { GhostCamera->Destroy(); GhostCamera = nullptr; }
		bHandoffInProgress = false;
		OnHandoffComplete.Broadcast();
		return;
	}

	// ── 1. TELEPORT PLAYER ────────────────────────────────────────────────────
	// Player is still hidden. Camera is at head bone. Teleport is invisible.
	CachedPlayer->SetActorLocationAndRotation(
		PendingPlayerSpawn.GetLocation(),
		PendingPlayerSpawn.GetRotation().Rotator(),
		false, nullptr, ETeleportType::TeleportPhysics);

	CachedPC->SetControlRotation(PendingPlayerSpawn.GetRotation().Rotator());

	// ── 2. RESTORE MOVEMENT MODE ──────────────────────────────────────────────
	if (UCharacterMovementComponent* Mv = CachedPlayer->GetCharacterMovement())
		if (Mv->MovementMode == MOVE_None)
			Mv->SetMovementMode(MOVE_Walking);

	// ── 3. DESTROY SK PROXY ───────────────────────────────────────────────────
	// Remove BEFORE unhiding player so both meshes never overlap.
	if (PendingProxyToDestroy && IsValid(PendingProxyToDestroy))
	{
		PendingProxyToDestroy->Destroy();
		PendingProxyToDestroy = nullptr;
	}

	// ── 4. UNHIDE PLAYER ──────────────────────────────────────────────────────
	// Camera is at head bone == player FP cam socket. Unhide is invisible.
	CachedPlayer->SetActorHiddenInGame(false);
	if (USkeletalMeshComponent* Mesh = CachedPlayer->GetMesh())
		Mesh->SetVisibility(true, true);

	// ── 5. CLEAR BLACK BARS (again, after proxy destruction) ──────────────────
	if (UCameraComponent* FPCam = CachedPlayer->FindComponentByClass<UCameraComponent>())
	{
		FPCam->bConstrainAspectRatio = false;
		FPCam->PostProcessSettings.bOverride_VignetteIntensity = false;
	}

	// ── 6. SWITCH PC VIEW TO PLAYER (instant snap) ────────────────────────────
	// Ghost position == head bone == FP cam → zero blend is seamless.
	CachedPC->SetViewTargetWithBlend(CachedPlayer, 0.0f);

	// ── 7. DESTROY GHOST ──────────────────────────────────────────────────────
	if (GhostCamera) { GhostCamera->Destroy(); GhostCamera = nullptr; }

	bHandoffInProgress = false;

	UE_LOG(LogTemp, Log,
		TEXT("UCinematicHandoffComponent: Arrived. Player at %s. Handoff complete."),
		*PendingPlayerSpawn.GetLocation().ToString());

	OnHandoffComplete.Broadcast();
}

// ─────────────────────────────────────────────────────────────────────────────
// SafetyRestoreInput — fires only if OnArrivalAtTarget never ran
// ─────────────────────────────────────────────────────────────────────────────

void UCinematicHandoffComponent::SafetyRestoreInput()
{
	UE_LOG(LogTemp, Warning,
		TEXT("UCinematicHandoffComponent::SafetyRestoreInput — Safety timer fired! "
		     "Handoff did not complete normally. Forcing player control restoration. "
		     "Check: Was BeginHandoff called? Was ProxySkeletonTag found?"));

	SetComponentTickEnabled(false);

	if (GhostCamera) { GhostCamera->Destroy(); GhostCamera = nullptr; }

	if (CachedPlayer)
	{
		CachedPlayer->SetActorHiddenInGame(false);
		if (USkeletalMeshComponent* Mesh = CachedPlayer->GetMesh())
			Mesh->SetVisibility(true, true);

		if (UCameraComponent* FPCam = CachedPlayer->FindComponentByClass<UCameraComponent>())
			FPCam->bConstrainAspectRatio = false;

		if (UCharacterMovementComponent* Mv = CachedPlayer->GetCharacterMovement())
			if (Mv->MovementMode == MOVE_None)
				Mv->SetMovementMode(MOVE_Walking);
	}

	if (CachedPC)
	{
		if (CachedPlayer)
			CachedPC->SetViewTargetWithBlend(CachedPlayer, 0.f);
		CachedPC->SetInputMode(FInputModeGameOnly());
		CachedPC->ResetIgnoreMoveInput();
	}

	bHandoffInProgress = false;
	OnHandoffComplete.Broadcast();
}

// ─────────────────────────────────────────────────────────────────────────────
// CancelHandoff
// ─────────────────────────────────────────────────────────────────────────────

void UCinematicHandoffComponent::CancelHandoff()
{
	SetComponentTickEnabled(false);

	if (UWorld* World = GetWorld())
		World->GetTimerManager().ClearTimer(SafetyHandle);

	if (GhostCamera) { GhostCamera->Destroy(); GhostCamera = nullptr; }

	bHandoffInProgress = false;
	UE_LOG(LogTemp, Log, TEXT("UCinematicHandoffComponent::CancelHandoff."));
}
