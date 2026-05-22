#include "PlayerSpawnMarker.h"
#include "Components/ArrowComponent.h"
#include "Components/BillboardComponent.h"

APlayerSpawnMarker::APlayerSpawnMarker()
{
	PrimaryActorTick.bCanEverTick = false;
	SetHidden(false); // visible in editor

	// ── ROOT ──────────────────────────────────────────────────────────────────
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// ── ARROW — shows position + facing direction in the editor ───────────────
	Arrow = CreateDefaultSubobject<UArrowComponent>(TEXT("DirectionArrow"));
	Arrow->SetupAttachment(Root);
	Arrow->SetArrowColor(FLinearColor(0.0f, 1.0f, 0.2f, 1.0f)); // bright green
	Arrow->ArrowSize           = 2.0f;
	Arrow->ArrowLength         = 100.0f;
	Arrow->bHiddenInGame       = true;
	Arrow->bTreatAsASprite     = false;

	// ── BILLBOARD — easier to click on in a crowded level ────────────────────
	Billboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
	Billboard->SetupAttachment(Root);
	Billboard->bHiddenInGame = true;
	Billboard->SetRelativeScale3D(FVector(0.5f));
}

void APlayerSpawnMarker::BeginPlay()
{
	Super::BeginPlay();

	// Fully invisible at runtime — editor only.
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}

FTransform APlayerSpawnMarker::GetSpawnTransform() const
{
	// Return the actor transform but with scale forced to 1.
	// We never want to scale the player pawn.
	return FTransform(GetActorRotation(), GetActorLocation(), FVector::OneVector);
}
