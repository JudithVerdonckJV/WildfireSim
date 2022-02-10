#include "ChildCell.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"

#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

AChildCell::AChildCell()
	: CellBounds{}
	, Trunk{}
	, Crown{}
	, MaxFuel{15}
	, Fuel{15}
	, MaxFireResistance{14}
	, FireResistance{14}
	, WindVelocity{}
	, TickInterval{0.5f}
	, FirstTickTimerHandle{}
	, RegrowRate{}
	, bCanRegrow{}
	, RegrowthTimerHandle{}
	, bUseVFX{true}
	, bIsIgnitionPoint{}
	, StartingState{ECellState::Normal}
	, CellState{ECellState::Normal}
	, NeighbourhingChildCells{}
	, NormalizedNeighbourDirections{}
	, CellSize{}
	, TerrainIntersectionPoint{}
	, TerrainIntersectionRotation{}
{
 	PrimaryActorTick.bCanEverTick = true;
	SetActorTickInterval(TickInterval);
	SetActorTickEnabled(false);

	CellBounds = CreateDefaultSubobject<UBoxComponent>("CellBounds");
	CellBounds->SetHiddenInGame(false);
	CellBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CellBounds->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	CellBounds->SetGenerateOverlapEvents(true);
	
	Trunk = CreateDefaultSubobject<UStaticMeshComponent>("Trunk");
	Trunk->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Crown = CreateDefaultSubobject<UStaticMeshComponent>("Crown");
	Crown->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	SetRootComponent(CellBounds);
	Trunk->SetupAttachment(CellBounds);
	Crown->SetupAttachment(CellBounds);
}

/// <summary>
/// Tick only runs whenever the cell is on fire, hence no state checking.
/// Once fuel is gone, tick is turned off again.
/// This causes damage to other cells and makes the current cell burn out.
/// </summary>
/// <param name="DeltaTime"></param>
void AChildCell::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	DamageNeighbourhingCells();
	Fuel--;
	if (Fuel <= 0)
	{
		SetCellBurnedOut();
	}
}

void AChildCell::SetFuel(int fuel)
{
	MaxFuel = fuel;
	Fuel = MaxFuel;
}

void AChildCell::SetFireResistance(int resistance)
{
	MaxFireResistance = resistance;
	FireResistance = MaxFireResistance;
}

void AChildCell::SetStartingState(bool isObstacle)
{
	if (isObstacle)
	{
		StartingState = ECellState::Obstacle;
		CellState = StartingState;
		return;
	}

	StartingState = ECellState::Normal;
	CellState = StartingState;
}

void AChildCell::SetCellSize(float size)
{
	CellSize = size;
	CellBounds->SetBoxExtent(FVector{ CellSize / 2.f, CellSize / 2.f, CellSize / 2.f });
}

void AChildCell::SetCellBoundsHidden(bool hidden)
{
	CellBounds->bHiddenInGame = hidden;
}

void AChildCell::SetTickInterval(float interval)
{
	SetActorTickInterval(interval);
	TickInterval = interval;
	//HandleFirstTickTimer();
}

/// <summary>
/// Use this to determine the averaged out intersection of the cell (cube) with any other shape.
/// Do not use on hotpaths!
/// Can be used for e.g. fire effects and vegetation.
/// </summary>
void AChildCell::FindTerrainIntersection()
{
	struct FEdge
	{
		FEdge(int index0, int index1)
			: Index0{ index0 }
			, Index1{ index1 }
		{
		}

		int Index0;
		int Index1;
	};	
	
	//First all the edges need to be found. For this, first get the vertices (cellCorners),
	//then determine all the edges with a sort of indexbuffer (cellEdges).
	const int vertexArraySize{ 8 };
	const int edgeArraySize{ 12 };
	const float cellHalfSize{ CellSize / 2.f };
	
	FVector cellCorners[vertexArraySize]{ {GetActorLocation() + FVector{-cellHalfSize, cellHalfSize, cellHalfSize}},
									{GetActorLocation() + FVector{-cellHalfSize, -cellHalfSize, cellHalfSize}},
									{GetActorLocation() + FVector{cellHalfSize, -cellHalfSize, cellHalfSize}},
									{GetActorLocation() + FVector{cellHalfSize, cellHalfSize, cellHalfSize}},
									{GetActorLocation() + FVector{-cellHalfSize, cellHalfSize, -cellHalfSize}},
									{GetActorLocation() + FVector{-cellHalfSize, -cellHalfSize, -cellHalfSize}},
									{GetActorLocation() + FVector{cellHalfSize, -cellHalfSize, -cellHalfSize}},
									{GetActorLocation() + FVector{cellHalfSize, cellHalfSize, -cellHalfSize}} };
	FEdge cellEdges[edgeArraySize]{ FEdge{0, 1}, FEdge{0, 3}, FEdge{0, 4}, FEdge{2, 1}, FEdge{2, 3}, FEdge{2, 6}, 
									FEdge{7, 4}, FEdge{7, 3}, FEdge{7, 6}, FEdge{5, 1}, FEdge{5, 4}, FEdge{5, 6}};

	//Linetrace every edge in both directions.
	//Reason for this is that UE does not register a linetrace starting inside a mesh going to the outside
	//as a blocking hit. Do not use the option "2-sided geometry" on the terrain mesh, this will cause the impact normals
	//to point in aswell as out!
	TArray<FVector> intersectionPoints{};
	TArray<FVector> intersectionNormals{};

	for (int i{}; i < edgeArraySize; ++i)
	{
		const FEdge currentEdge{ cellEdges[i] };
		
		FHitResult hitResult{};
		FCollisionQueryParams params{};
		params.AddIgnoredActor(this);
		params.bTraceComplex = true;
		const FVector start{ cellCorners[currentEdge.Index0]};
		const FVector end{ cellCorners[currentEdge.Index1] };

		GetWorld()->LineTraceSingleByChannel(hitResult, start, end, ECollisionChannel::ECC_GameTraceChannel1, params);

		if (hitResult.bBlockingHit)
		{
			intersectionPoints.Add(hitResult.ImpactPoint);
			intersectionNormals.Add(hitResult.ImpactNormal);
			//if a blocking hit was already found, do not trace the other direction.
			continue;
		}

		GetWorld()->LineTraceSingleByChannel(hitResult, end, start, ECollisionChannel::ECC_GameTraceChannel1, params);

		if (hitResult.bBlockingHit)
		{
			intersectionPoints.Add(hitResult.ImpactPoint);
			intersectionNormals.Add(hitResult.ImpactNormal);
		}
	}
	
	if (intersectionPoints.Num() == 0 || intersectionNormals.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("No intersections were found for childcell. Childcell should probably have been eliminated!"));
		return;
	}

	//Average out the gathered locations and normals. This will then give the best intersection point and rotation
	//to use for fire and trees etc.
	FVector resultImpactPoint{0.f, 0.f, 0.f};
	for (const FVector& point : intersectionPoints)
	{
		resultImpactPoint += point;
	}
	resultImpactPoint /= intersectionPoints.Num();
	TerrainIntersectionPoint = resultImpactPoint;

	FVector resultImpactNormal{0.f, 0.f, 0.f};
	for (const FVector& normal : intersectionNormals)
	{
		resultImpactNormal += normal;
	}
	resultImpactNormal /= intersectionNormals.Num();
	TerrainIntersectionRotation = resultImpactNormal.Rotation();
	TerrainIntersectionRotation.Pitch -= 90.f;
}

void AChildCell::UseTerrainIntersection()
{
	SetFireRotationAndLocation();
	Trunk->SetWorldLocationAndRotation(TerrainIntersectionPoint, TerrainIntersectionRotation);
	Crown->SetWorldLocationAndRotation(TerrainIntersectionPoint, TerrainIntersectionRotation);
}

/// <summary>
/// Uses overlap of a slightly bigger box to determine the neighbours. This makes sure there is no risk of
/// out-of-bounds errors and oddities when trying to get neighbourhing cells from the 1D array after cells
/// have been eliminated.
/// </summary>
void AChildCell::FindNeighbours()
{
	TArray<AActor*> overlappedActors{};
	FVector halfExtents{ CellBounds->GetScaledBoxExtent() };
	halfExtents *= 1.1f;
	UKismetSystemLibrary::BoxOverlapActors(GetWorld(), GetActorLocation(), halfExtents, {}, AChildCell::StaticClass(), { this }, overlappedActors);

	for (AActor* const actor : overlappedActors)
	{
		AChildCell* neighbourCell{ Cast<AChildCell>(actor) };
		if (neighbourCell)
		{
			NeighbourhingChildCells.Add(neighbourCell);
		}
	}

	for (const AChildCell* const cell : NeighbourhingChildCells)
	{
		FVector fireDirection{ (cell->GetActorLocation() - GetActorLocation()) };
		fireDirection.Normalize();
		NormalizedNeighbourDirections.Add(fireDirection);
	}
}

void AChildCell::TakeFireDamage(float damage)
{
	if (CellState == ECellState::Normal)
	{
		FireResistance -= damage;
		if (FireResistance <= 0)
		{
			SetCellOnFire();
			HandleFirstTickTimer();
		}
	}
}

void AChildCell::DamageNeighbourhingCells()
{
	for (int i{}; i < NeighbourhingChildCells.Num(); ++i)
	{
		float damageMod{ FVector::DotProduct(NormalizedNeighbourDirections[i], WindVelocity) };

		if (damageMod < 1.f)
		{
			damageMod = 1.f; //always damage cells a little bit, never deal negative damage
		}

		NeighbourhingChildCells[i]->TakeFireDamage(damageMod);
	}
}

void AChildCell::SetCellOnFire()
{
	CellState = ECellState::Burning;
	if(bUseVFX) HandleFireVFX(true);
	Crown->SetVisibility(false);

	if (bIsIgnitionPoint)
	{
		SetActorTickEnabled(true);
	}
}

void AChildCell::Reset()
{
	Fuel = MaxFuel;
	FireResistance = MaxFireResistance;
	CellState = StartingState;
	HandleFireVFX(false);
	Trunk->SetVisibility(true);
	Crown->SetVisibility(true);
	GetWorldTimerManager().ClearTimer(FirstTickTimerHandle);
	SetActorTickEnabled(false);
}

void AChildCell::ResetOnlyWhenBurnedOut()
{
	if (CellState == ECellState::Burned)
	{
		Reset();
	}
}

void AChildCell::SetCellBurnedOut()
{
	CellState = ECellState::Burned;
	SetActorTickEnabled(false);
	HandleFireVFX(false);
	Trunk->SetVisibility(false);

	if (bCanRegrow)
	{
		HandleRegrowthTimer();
	}
}


/// <summary>
/// When a Tick gets enables, it gets fired at its normal fps: this means that if the pc is running the app at
/// 144 fps, the tick would already fire after 0.007 second instead of after its assigned TickInterval.
/// In the wildfire sim, this behavior is unwanted, as this causes a chainreaction of all cells immediatly
/// catching on fire within a second (depending on the fuel and resistance variables aswell).
/// This timer is set to enable the Tick after the right interval, so the interval is kept consistent throughout
/// the simulation.
/// </summary>
void AChildCell::HandleFirstTickTimer()
{
	GetWorldTimerManager().ClearTimer(FirstTickTimerHandle);
	FTimerDelegate timerDel{};
	timerDel.BindUFunction(this, FName("SetActorTickEnabled"), true);
	GetWorldTimerManager().SetTimer(FirstTickTimerHandle, timerDel, TickInterval, false);
}

void AChildCell::HandleRegrowthTimer()
{
	GetWorldTimerManager().ClearTimer(RegrowthTimerHandle);
	FTimerDelegate timerDel{};
	timerDel.BindUFunction(this, FName("Reset"));
	GetWorldTimerManager().SetTimer(RegrowthTimerHandle, timerDel, RegrowRate, false);
}