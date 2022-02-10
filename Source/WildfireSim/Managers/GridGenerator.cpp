#include "GridGenerator.h"

#include "Components/BoxComponent.h"
#include "../Cells/ParentCell.h"
#include "../Cells/ChildCell.h"

#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

AGridGenerator::AGridGenerator()
	: ParentCellClass{}
	, Rows{ }
	, Columns{ }
	, Depth{ }
	, ParentCellSize{ 1000.f }
	, ChildCellNumber{ 3 }
	, Fuel{10}
	, Resistance{9}
	, ParentGrid{}
	, ActorsToEnvelop{}
{
 	PrimaryActorTick.bCanEverTick = false;
}
void AGridGenerator::StopFireSimulation()
{
	for (AParentCell* cell : ParentGrid)
	{
		cell->ResetChildGrid();
	}
}

void AGridGenerator::SetTickRateOnGrid(float tickRate)
{
	for (AParentCell* cell : ParentGrid)
	{
		cell->SetChildGridTickRate(tickRate);
	}
}

void AGridGenerator::SetFuelOnGrid(int fuel)
{
	Fuel = fuel;

	for (AParentCell* const cell : ParentGrid)
	{
		cell->SetBaseFuel(fuel);
	}
}

void AGridGenerator::SetResistanceOnGrid(int resistance)
{
	Resistance = resistance;

	for (AParentCell* const cell : ParentGrid)
	{
		cell->SetBaseFireResistance(resistance);
	}
}

void AGridGenerator::SetWindVelocityOnGrid(const FVector& windVel)
{
	for (AParentCell* cell : ParentGrid)
	{
		cell->SetWindVelocity(windVel);
	}
}

AParentCell* const AGridGenerator::FindPickedParentCell(const FVector& position) const
{
	for (AParentCell* const cell : ParentGrid)
	{
		const UBoxComponent* const bounds = cell->GetBounds();
		bool isInBox = UKismetMathLibrary::IsPointInBox(position, bounds->GetComponentLocation(), bounds->GetScaledBoxExtent());
		
		if (isInBox)
		{
			return cell;
		}
	}

	return nullptr;
}

void AGridGenerator::RegrowBurnedTrees()
{
	for (AParentCell* const cell : ParentGrid)
	{
		cell->RegrowBurnedTrees();
	}
}

void AGridGenerator::SetRegrowRate(float rate)
{
	for (AParentCell* const cell : ParentGrid)
	{
		cell->SetRegrowRate(rate);
	}
}

void AGridGenerator::EnableRegrowth(bool enabled)
{
	for (AParentCell* const cell : ParentGrid)
	{
		cell->EnableRegrowth(enabled);
	}
}

//GRID
void AGridGenerator::GenerateGrids()
{
	GenerateParentGrid();
	GenerateChildGrid();
	FindChildCellNeighbours();
}

/// <summary>
/// This will:
/// 1) Reset all the arrays with cells, both child and parent cells.
/// 2) Figure out how many rows, columns and depth is needed in order to completely envelop
///	   the given terrain actors. The cell size that was given by the user does not change.
/// 3) Do a box overlap on the place where the cell would be spawned. If there is overlap with the terrain it will go on to
/// 4) actually spawn in the cell.
/// 5) Set all stats on the new cell.
/// </summary>
void AGridGenerator::GenerateParentGrid()
{
	//RESET
	for (AParentCell* cell : ParentGrid)
	{		
		cell->EmptyChildGrid();
		cell->Destroy();
	}
	ParentGrid.Empty();
	
	//BOUNDS AND ROW/COL/DEPTH
	FVector origin{};
	FVector terrainHalfExtents{};
	FindMaxBounds(origin, terrainHalfExtents);

	Columns = int(terrainHalfExtents.X * 2.f / ParentCellSize) + 2; // + 2 is to account for any rounding that happens. On top of that, cells
	Depth = int(terrainHalfExtents.Y * 2.f / ParentCellSize) + 2;   // will line up their center, not their extent to the actors.
	Rows = int(terrainHalfExtents.Z * 2.f / ParentCellSize) + 2;

	//CELL PLACEMENT
	FVector cellLocation{ };
	cellLocation.X = origin.X - terrainHalfExtents.X;
	cellLocation.Y = origin.Y - terrainHalfExtents.Y;
	cellLocation.Z = origin.Z - terrainHalfExtents.Z;
	
	for (int row{}; row < Rows; ++row)
	{
		for (int column{}; column < Columns; ++column)
		{
			for (int depth{}; depth < Depth; ++depth)
			{
				//Do a box overlap with the terrain actors. If there is an overlap, this cell has to stay.
				TArray<AActor*> overlappedActors{};
				const FVector cellHalfExtents{ ParentCellSize / 2.f, ParentCellSize / 2.f, ParentCellSize / 2.f };
				bool bHasOverlapWithTerrain{};

				UKismetSystemLibrary::BoxOverlapActors(GetWorld(), cellLocation, cellHalfExtents, {}, AActor::StaticClass(), { this }, overlappedActors);
				for (AActor* const actor : overlappedActors)
				{
					for (const AActor* terrain : ActorsToEnvelop)
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
					AParentCell* newCell{ GetWorld()->SpawnActor<AParentCell>(ParentCellClass, cellLocation, { 0.f, 0.f, 0.f }, spawnParams) };

					ParentGrid.Add(newCell);
					newCell->SetCellSize(ParentCellSize);
					
					newCell->SetBaseFuel(Fuel);
					newCell->SetBaseFireResistance(Resistance);
					newCell->SetChildCellNumber(ChildCellNumber);
				}

				cellLocation.Y += ParentCellSize;
			}

			cellLocation.Y = origin.Y - terrainHalfExtents.Y;
			cellLocation.X += ParentCellSize;
		}
		cellLocation.Z += ParentCellSize;
		cellLocation.X = origin.X - terrainHalfExtents.X;
	}
}

void AGridGenerator::GenerateChildGrid()
{
	for (AParentCell* cell : ParentGrid)
	{
		cell->SpawnChildGrid(ActorsToEnvelop);
	}
}

/// <summary>
/// Only call this after the entire grid has been generated!
/// </summary>
void AGridGenerator::FindChildCellNeighbours()
{
	for (AParentCell* const cell : ParentGrid)
	{
		cell->FindChildNeighbours();
	}
}

void AGridGenerator::FindMaxBounds(FVector& origin, FVector& halfExtents) const
{
	FVector currentOrigin{};
	FVector currentHalfExtents{};

	FVector leftBottomBack{};
	FVector rightTopFront{};
	
	for (const AActor* actor : ActorsToEnvelop)
	{
		actor->GetActorBounds(false, currentOrigin, currentHalfExtents, true);

		//LEFT BOTTOM BACK
		const FVector newLeftBottomBack{ currentOrigin - currentHalfExtents };
		CheckAndReplaceBound_smaller(newLeftBottomBack.X, leftBottomBack.X);
		CheckAndReplaceBound_smaller(newLeftBottomBack.Y, leftBottomBack.Y);
		CheckAndReplaceBound_smaller(newLeftBottomBack.Z, leftBottomBack.Z);

		//RIGHT TOP FRONT
		const FVector newRightTopFront{ currentOrigin + currentHalfExtents };
		CheckAndReplaceBound_greater(newRightTopFront.X, rightTopFront.X);
		CheckAndReplaceBound_greater(newRightTopFront.Y, rightTopFront.Y);
		CheckAndReplaceBound_greater(newRightTopFront.Z, rightTopFront.Z);
	}

	halfExtents.X = (rightTopFront.X - leftBottomBack.X) / 2.f;
	halfExtents.Y = (rightTopFront.Y - leftBottomBack.Y) / 2.f;
	halfExtents.Z = (rightTopFront.Z - leftBottomBack.Z) / 2.f;

	origin = leftBottomBack + halfExtents;
}

void AGridGenerator::CheckAndReplaceBound_greater(const float newValue, float& oldValue) const
{
	if (newValue > oldValue)
	{
		oldValue = newValue;
	}
}

void AGridGenerator::CheckAndReplaceBound_smaller(const float newValue, float& oldValue) const
{
	if (newValue < oldValue)
	{
		oldValue = newValue;
	}
}

void AGridGenerator::SetCanUseVFX(bool canUse)
{
	for (AParentCell* const cell : ParentGrid)
	{
		cell->SetCanUseVFX(canUse);
	}
}
