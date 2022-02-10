#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridGenerator.generated.h"

class AParentCell;
class AChildCell;

UCLASS()
class WILDFIRESIM_API AGridGenerator : public AActor
{
	GENERATED_BODY()
	
public:	
	AGridGenerator();
		
	UFUNCTION(BlueprintCallable)
		void GenerateGrids();

	void StopFireSimulation();

	FORCEINLINE void SetParentCellSize(float size) { ParentCellSize = size; };
	FORCEINLINE void SetChildCellNumber(int nr) { ChildCellNumber = nr; };
	void SetTickRateOnGrid(float tickRate);
	void SetFuelOnGrid(int fuel);
	void SetResistanceOnGrid(int resistance);
	void SetWindVelocityOnGrid(const FVector& windVel);

	AParentCell* const FindPickedParentCell(const FVector& position) const;
	void RegrowBurnedTrees();

	void SetRegrowRate(float rate);
	void EnableRegrowth(bool enabled);

	void SetCanUseVFX(bool canUse);

protected:
	UPROPERTY(EditAnywhere)
		TSubclassOf<AParentCell> ParentCellClass;

	void GenerateParentGrid();
	void GenerateChildGrid();
	void FindChildCellNeighbours();

	int Rows;
	int Columns;
	int Depth;

	UPROPERTY(EditAnywhere)
		float ParentCellSize;
	int ChildCellNumber;

	int Fuel;
	int Resistance;

	UPROPERTY(EditAnywhere)
		TArray<AActor*> ActorsToEnvelop;

private:
	TArray<AParentCell*> ParentGrid;

	//HELPERS
	void FindMaxBounds(FVector& origin, FVector& halfExtents) const;
	void CheckAndReplaceBound_greater(const float newValue, float& oldValue) const;
	void CheckAndReplaceBound_smaller(const float newValue, float& oldValue) const;
};
