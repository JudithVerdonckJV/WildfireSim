#include "ParentCell.h"
#include "ChildCell.h"

#include "Components/BoxComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

#include "DrawDebugHelpers.h"

AParentCell::AParentCell()
	: ChildCellClass{}
	, BaseFuel{15}
	, BaseFireResistance{14}
	, WindVelocity{}
	, CellSize{}
	, CellBounds{}
	, ChildGrid{}
	, ChildRows{ 4 }
	, ChildSize{}
{
 	PrimaryActorTick.bCanEverTick = false;

	CellBounds = CreateDefaultSubobject<UBoxComponent>("CellBounds");
	CellBounds->SetHiddenInGame(false);
	CellBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CellBounds->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	CellBounds->SetGenerateOverlapEvents(true);

	SetRootComponent(CellBounds);
}

void AParentCell::SetBaseFuel(int fuel)
{
	BaseFuel = fuel;
	for (AChildCell* const cell : ChildGrid)
	{
		cell->SetFuel(BaseFuel);
	}
}

void AParentCell::SetBaseFireResistance(int resistance)
{
	BaseFireResistance = resistance;
	for (AChildCell* const cell : ChildGrid)
	{
		cell->SetFireResistance(BaseFireResistance);
	}
}

void AParentCell::SetWindVelocity(const FVector& velocity)
{
	WindVelocity = velocity;
	for (AChildCell* const cell : ChildGrid)
	{
		cell->SetWindVelocity(WindVelocity);
	}
}

void AParentCell::SetWindVelocity(float strength, const FVector& direction)
{
	const FVector normalizedDirection{ UKismetMathLibrary::Normal(direction) };
	WindVelocity = strength * normalizedDirection;
	
	for (AChildCell* const cell : ChildGrid)
	{
		cell->SetWindVelocity(WindVelocity);
	}
}

void AParentCell::SetCellSize(float size)
{
	CellSize = size;
	CellBounds->SetBoxExtent(FVector{ CellSize / 2.f, CellSize / 2.f, CellSize / 2.f });
}

void AParentCell::SetCellBoundsHidden(bool hidden)
{
	CellBounds->bHiddenInGame = hidden;
}

void AParentCell::SetChildCellBoundsHidden(bool hidden)
{
	for (AChildCell* const cell : ChildGrid)
	{
		cell->SetCellBoundsHidden(hidden);
	}
}

void AParentCell::SetChildGridTickRate(float tickRate)
{
	for (AChildCell* cell : ChildGrid)
	{
		cell->SetTickInterval(tickRate);
	}
}

void AParentCell::FindChildNeighbours()
{
	for (AChildCell* const cell : ChildGrid)
	{
		cell->UpdateOverlaps();
		cell->FindNeighbours();
	}
}

/// <summary>
/// This will:
/// 1) Figure out how large a child cell needs to be.
/// 2) Do a box overlap on the place where the cell would be spawned. If there is overlap with the terrain it will go on to
/// 3) actually spawn in the cell.
/// 4) Set all stats on the new cell.
/// </summary>
/// <param name="actorsToEnvelop"></param>
void AParentCell::SpawnChildGrid(const TArray<AActor*>& actorsToEnvelop)
{
	ChildSize = CellSize / ChildRows;
	
	const FVector& start{GetActorLocation()};
	FVector cellLocation{};
	cellLocation.X = start.X - (CellSize / 2.f) + ChildSize / 2.f;
	cellLocation.Y = start.Y - (CellSize / 2.f) + ChildSize / 2.f;
	cellLocation.Z = start.Z - (CellSize / 2.f) + ChildSize / 2.f;

	//Spawn cells
	for (int row{}; row < ChildRows; ++row)
	{
		for (int column{}; column < ChildRows; ++column)
		{
			for (int depth{}; depth < ChildRows; ++depth)
			{
				//Do a box overlap with the terrain actors. If there is an overlap, this cell has to stay.
				TArray<AActor*> overlappedActors{};
				const FVector halfExtents{ ChildSize / 2.f, ChildSize / 2.f, ChildSize / 2.f };
				bool bHasOverlapWithTerrain{};

				UKismetSystemLibrary::BoxOverlapActors(GetWorld(), cellLocation, halfExtents, {}, AActor::StaticClass(), { this }, overlappedActors);
				for (AActor* const actor : overlappedActors)
				{
					for (const AActor* terrain : actorsToEnvelop)
					{
						if (actor == terrain)
						{
							bHasOverlapWithTerrain = true;
							break; // No need to check the others
						}
					}

					if (bHasOverlapWithTerrain) break;
				}

				//If there is overlap with the terrain, actually spawn in the cell.
				if (bHasOverlapWithTerrain)
				{
					FActorSpawnParameters spawnParams{};
					spawnParams.Owner = this;
					spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
					AChildCell* newCell = GetWorld()->SpawnActor<AChildCell>(ChildCellClass, cellLocation, { 0.f, 0.f, 0.f }, spawnParams);

					ChildGrid.Add(newCell);
					newCell->SetCellSize(ChildSize);

					//Set all the stats
					newCell->SetFuel(BaseFuel);
					newCell->SetFireResistance(BaseFireResistance);
					newCell->SetWindVelocity(WindVelocity);
					newCell->SetStartingState(false);
					newCell->SetActorTickEnabled(false);
					
					newCell->FindTerrainIntersection();
					newCell->UseTerrainIntersection();
				}

				cellLocation.Y += ChildSize;
			}

			cellLocation.Y = start.Y - (CellSize / 2.f) + ChildSize / 2.f;
			cellLocation.X += ChildSize;
		}
		cellLocation.Z += ChildSize;
		cellLocation.X = start.X - (CellSize / 2.f) + ChildSize / 2.f;
	}
}

void AParentCell::ResetChildGrid()
{
	for (AChildCell* cell : ChildGrid)
	{
		cell->Reset();
	}
}

void AParentCell::EmptyChildGrid()
{
	for (AChildCell* cell : ChildGrid)
	{
		cell->Destroy();
	}
	ChildGrid.Empty();
}

AChildCell* const AParentCell::GetPickedChildCell(const FVector& position) const
{
	for (AChildCell* const cell : ChildGrid)
	{
		bool isInBox = UKismetMathLibrary::IsPointInBox(position, cell->GetActorLocation(), {ChildSize/2.f, ChildSize/2.f, ChildSize/2.f});

		if (isInBox)
		{
			return cell;
		}
	}

	return nullptr;
}

void AParentCell::RegrowBurnedTrees()
{
	for (AChildCell* const cell : ChildGrid)
	{
		cell->ResetOnlyWhenBurnedOut();
	}
}

void AParentCell::SetRegrowRate(float rate)
{
	for (AChildCell* const cell : ChildGrid)
	{
		cell->SetRegrowRate(rate);
	}
}

void AParentCell::EnableRegrowth(bool enabled)
{
	for (AChildCell* const cell : ChildGrid)
	{
		cell->EnableRegrowth(enabled);
	}
}

void AParentCell::SetCanUseVFX(bool canUse)
{
	for (AChildCell* const cell : ChildGrid)
	{
		cell->SetCanUseVFX(canUse);
	}
}