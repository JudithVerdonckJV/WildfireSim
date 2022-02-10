#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ParentCell.generated.h"

class UBoxComponent;
class AChildCell;

UCLASS()
class WILDFIRESIM_API AParentCell : public AActor
{
	GENERATED_BODY()
	
public:	
	AParentCell();

	void SetBaseFuel(int fuel);
	void SetBaseFireResistance(int resistance);
	void SetWindVelocity(float strenght, const FVector& direction);
	void SetWindVelocity(const FVector& velocity);
	FORCEINLINE const FVector& GetWindVelocity() const { return WindVelocity; };
	void SetCellSize(float size);
	void SetCellBoundsHidden(bool hidden);
	void SetChildCellBoundsHidden(bool hidden);
	FORCEINLINE void SetChildCellNumber(int nr) { ChildRows = nr; };
	void SetChildGridTickRate(float tickRate);
	
	void FindChildNeighbours();
	void SpawnChildGrid(const TArray<AActor*>& actorsToEnvelop);

	void ResetChildGrid();
	void EmptyChildGrid();

	FORCEINLINE const UBoxComponent* const GetBounds() const { return CellBounds; };
	AChildCell* const GetPickedChildCell(const FVector& position) const;
	void RegrowBurnedTrees();

	void SetRegrowRate(float rate);
	void EnableRegrowth(bool enabled);

	void SetCanUseVFX(bool canUse);

protected:
	UPROPERTY(EditAnywhere)
		TSubclassOf<AChildCell> ChildCellClass;

	int BaseFuel;
	int BaseFireResistance;
	FVector WindVelocity;
	float CellSize;

	UBoxComponent* CellBounds;

	TArray<AChildCell*> ChildGrid;
	int ChildRows;
	float ChildSize;
};
