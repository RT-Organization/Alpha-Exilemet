#include "SkullProp.h"
#include "TutorialDirector.h"

#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/Pawn.h"

ASkullProp::ASkullProp()
{
	PrimaryActorTick.bCanEverTick       = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

// ─────────────────────────────────────────────────────────────────────────────
// BeginPlay
// ─────────────────────────────────────────────────────────────────────────────

void ASkullProp::BeginPlay()
{
	Super::BeginPlay();
	// Player pawn is fetched lazily in Tick to avoid timing issues with
	// the GameMode spawning the player after this actor's BeginPlay.
}

// ─────────────────────────────────────────────────────────────────────────────
// Tick — look-at + flicker
// ─────────────────────────────────────────────────────────────────────────────

void ASkullProp::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// ── LAZY PLAYER FETCH ────────────────────────────────────────────────────
	if (!PlayerPawn)
	{
		PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	}

	// ── LOOK AT PLAYER ───────────────────────────────────────────────────────
	// Yaw only: skull rotates horizontally to face the player.
	// Roll and Pitch are locked to zero so the skull never tilts.
	if (PlayerPawn)
	{
		const FRotator LookAt = UKismetMathLibrary::FindLookAtRotation(
			GetActorLocation(),
			PlayerPawn->GetActorLocation());

		const FRotator Current  = GetActorRotation();
		const FRotator Interped = FMath::RInterpTo(Current, LookAt, DeltaTime, LookAtInterpSpeed);

		SetActorRotation(FRotator(0.f, Interped.Yaw, 0.f));
	}

	// ── FLICKER ──────────────────────────────────────────────────────────────
	// Rapid random position jitter around SkullHomePosition.
	// Every tick while bFlickerActive the skull teleports to a new random
	// offset — producing the supernatural vibration effect.
	if (bFlickerActive && !SkullHomePosition.IsZero())
	{
		const FVector RandomOffset = FMath::VRand() * FlickerIntensity;
		SetActorLocation(SkullHomePosition + RandomOffset, false, nullptr, ETeleportType::TeleportPhysics);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────

void ASkullProp::SetDirector(ATutorialDirector* Director)
{
	DirectorRef = Director;
	UE_LOG(LogTemp, Log, TEXT("ASkullProp: DirectorRef set to %s"),
		Director ? *Director->GetName() : TEXT("null"));
}

void ASkullProp::OnRiseComplete()
{
	// 1. Save the skull's actual world position after the rise animation.
	//    This is what StopAndReset() will snap back to.
	SkullHomePosition = GetActorLocation();

	// 2. Enable interaction — the player can now press F to examine.
	bCanInteract = true;

	// 3. Notify BP for optional visual feedback (glow pulse, particle etc.).
	BP_OnBecameInteractable();

	UE_LOG(LogTemp, Log, TEXT("ASkullProp: Rise complete. HomePos=%s. Interaction enabled."),
		*SkullHomePosition.ToString());
}

void ASkullProp::StartFlicker()
{
	// Safety: if OnRiseComplete was never called (e.g. sequence was skipped),
	// save the current location now so flicker has a valid home to orbit.
	if (SkullHomePosition.IsZero())
	{
		SkullHomePosition = GetActorLocation();
		UE_LOG(LogTemp, Warning,
			TEXT("ASkullProp::StartFlicker — SkullHomePosition was zero. "
			     "Saved current location (%s) as fallback. "
			     "Make sure OnRiseComplete() is called from the rise Timeline Finished pin."),
			*SkullHomePosition.ToString());
	}

	bFlickerActive = true;
	UE_LOG(LogTemp, Log, TEXT("ASkullProp: Flicker started."));
}

void ASkullProp::StopAndReset()
{
	bFlickerActive = false;

	if (!SkullHomePosition.IsZero())
	{
		// TeleportPhysics: instant, no sweep, no physics interaction.
		// This happens under the black screen, so the snap is invisible.
		SetActorLocation(SkullHomePosition, false, nullptr, ETeleportType::TeleportPhysics);
		UE_LOG(LogTemp, Log, TEXT("ASkullProp: Flicker stopped. Snapped to HomePos=%s."),
			*SkullHomePosition.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("ASkullProp::StopAndReset — SkullHomePosition is zero. Skull was not moved."));
	}
}

void ASkullProp::HandleInteract()
{
	if (!bCanInteract)
	{
		// Interaction guard: skull is not ready yet, or was already interacted.
		return;
	}

	// Clear immediately to prevent double-interaction if the player
	// hammers the interact key during the frame the event fires.
	bCanInteract = false;

	if (!DirectorRef)
	{
		UE_LOG(LogTemp, Error,
			TEXT("ASkullProp::HandleInteract — DirectorRef is null! "
			     "Was ATutorialDirector::InitializeTutorial() called? "
			     "Did BP BeginPlay call SkullRef → SetDirector(Self)?"));
		return;
	}

	DirectorRef->OnSkullInteracted();
}
