#include "GasRodTool.h"
#include "GasSphere.h"
#include "AlphaExilemet/Core/AlphaExilemetSaveGame.h"

AGasRodTool::AGasRodTool()
{
	PrimaryActorTick.bCanEverTick = true;

	CapacityProgression.BaseValue        = 1.0f;
	CapacityProgression.AdditivePerLevel = 1.0f;

	AbsSpeedProgression.BaseValue        = 10.0f;
	AbsSpeedProgression.AdditivePerLevel = 2.0f;

	RangeProgression.BaseValue           = 1000.0f;
	RangeProgression.AdditivePerLevel    = 200.0f;
}

void AGasRodTool::BeginPlay()
{
	Super::BeginPlay();
}

void AGasRodTool::StartUsing_Implementation()
{
	Super::StartUsing_Implementation();

	if (!CanFireSphere() || !SphereClass) return;
	if (!TakeEmptySphere()) return;

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn) return;

	FVector SpawnLocation = GetActorLocation();
	FRotator SpawnRotation = OwnerPawn->GetControlRotation();

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.Instigator = OwnerPawn;

	AGasSphere* Sphere = GetWorld()->SpawnActor<AGasSphere>(
		SphereClass,
		SpawnLocation,
		SpawnRotation,
		Params
	);

	if (!Sphere) return;

	// AbsSpeed = DPS
	Sphere->InitSphere(this, GetAbsorptionSpeed());

	ActiveSpheres.Add(Sphere);

	UPrimitiveComponent* Root = Cast<UPrimitiveComponent>(Sphere->GetRootComponent());
	if (Root && Root->IsSimulatingPhysics())
	{
		FVector Dir = SpawnRotation.Vector();
		float Force = GetRodRange();

		Root->AddImpulse(Dir * Force, NAME_None, true);
	}
}

void AGasRodTool::StopUsing_Implementation()
{
	Super::StopUsing_Implementation();
}

// =========================================================================
// STATS
// =========================================================================

float AGasRodTool::GetAbsorptionSpeed() const
{
	int32 Level = ToolUpgradeLevels.FindRef(FName("Rod_AbsSpeed"));
	return AbsSpeedProgression.GetValueAtLevel(Level);
}

float AGasRodTool::GetRodRange() const
{
	int32 Level = ToolUpgradeLevels.FindRef(FName("Rod_Distance"));
	return RangeProgression.GetValueAtLevel(Level);
}

float AGasRodTool::GetMaxCapacity() const
{
	int32 Level = ToolUpgradeLevels.FindRef(FName("Rod_Quantity"));
	return CapacityProgression.GetValueAtLevel(Level);
}

// =========================================================================
// INVENTORY OPERATIONS
// =========================================================================

int32 AGasRodTool::AddEmptySpheresToRod(int32 Quantity)
{
	if (Quantity <= 0) return 0;

	int32 MaxSlots   = FMath::FloorToInt(GetMaxCapacity());
	int32 TotalInRod = GetTotalSpheresInRod();
	int32 FreeSlots  = FMath::Max(0, MaxSlots - TotalInRod);
	int32 ToAdd      = FMath::Min(Quantity, FreeSlots);

	EmptySphereCount += ToAdd;
	return ToAdd;
}

bool AGasRodTool::TakeEmptySphere()
{
	if (EmptySphereCount <= 0) return false;
	EmptySphereCount--;
	return true;
}

bool AGasRodTool::ReturnFullSphere(FName GasType)
{
	if (GasType.IsNone()) return false;
	if (IsRodFull()) return false;

	HarvestedGas.FindOrAdd(GasType)++;
	return true;
}

void AGasRodTool::RecallAllSpheres()
{
	for (auto& SpherePtr : ActiveSpheres)
	{
		if (SpherePtr.IsValid())
		{
			SpherePtr->Destroy(); // recall = remove sphere
		}
	}

	ActiveSpheres.Empty();
}

// =========================================================================
// INVENTORY QUERIES
// =========================================================================

int32 AGasRodTool::GetTotalSpheresInRod() const
{
	int32 Full = 0;
	for (const auto& Pair : HarvestedGas)
	{
		Full += Pair.Value;
	}
	return EmptySphereCount + Full;
}

bool AGasRodTool::CanFireSphere() const
{
	return EmptySphereCount > 0;
}

bool AGasRodTool::IsRodFull() const
{
	return GetTotalSpheresInRod() >= FMath::FloorToInt(GetMaxCapacity());
}

// =========================================================================
// INVENTORY SYSTEM
// =========================================================================

void AGasRodTool::ClearInventory(float RetainedFraction)
{
	EmptySphereCount = 0;

	if (RetainedFraction <= 0.0f)
	{
		HarvestedGas.Empty();
		return;
	}

	for (auto It = HarvestedGas.CreateIterator(); It; ++It)
	{
		int32 Retained = FMath::FloorToInt(It.Value() * RetainedFraction);
		if (Retained > 0) It.Value() = Retained;
		else              It.RemoveCurrent();
	}
}

TMap<FName, int32> AGasRodTool::GetAllResources() const
{
	return HarvestedGas;
}

int32 AGasRodTool::RemoveResource(FName InResourceID, int32 Amount)
{
	if (Amount <= 0 || !HarvestedGas.Contains(InResourceID)) return 0;

	int32 Current  = HarvestedGas[InResourceID];
	int32 ToRemove = FMath::Min(Current, Amount);

	HarvestedGas[InResourceID] -= ToRemove;

	if (HarvestedGas[InResourceID] <= 0)
	{
		HarvestedGas.Remove(InResourceID);
	}

	// consumed gas → empty spheres restored
	EmptySphereCount += ToRemove;

	return ToRemove;
}

int32 AGasRodTool::GetResourceAmount(FName InResourceID) const
{
	return HarvestedGas.Contains(InResourceID)
		? HarvestedGas[InResourceID]
		: 0;
}

// =========================================================================
// SAVE / LOAD
// =========================================================================

void AGasRodTool::SaveToolData(UAlphaExilemetSaveGame* SaveObject)
{
	Super::SaveToolData(SaveObject);
	if (!SaveObject) return;

	SaveObject->SavedHarvestedGas     = HarvestedGas;
	SaveObject->SavedEmptySphereCount = EmptySphereCount;
}

void AGasRodTool::LoadToolData(UAlphaExilemetSaveGame* SaveObject)
{
	Super::LoadToolData(SaveObject);
	if (!SaveObject) return;

	HarvestedGas     = SaveObject->SavedHarvestedGas;
	EmptySphereCount = SaveObject->SavedEmptySphereCount;
}