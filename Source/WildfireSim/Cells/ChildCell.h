#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ChildCell.generated.h"

class AParentCell;
class UBoxComponent;
class UStaticMeshComponent;

UENUM()
enum ECellState
{
	Normal		UMETA(DisplayName = "Normal"),
	Burning		UMETA(DisplayName = "Burning"),
	Burned		UMETA(DisplayName = "Burned"),
	Obstacle	UMETA(DisplayName = "Obstacle"),
};

UCLASS()
class WILDFIRESIM_API AChildCell : public AActor
{
	GENERATED_BODY()
	
public:	
	AChildCell();

	virtual void Tick(float DeltaTime) override;

	void SetFuel(int fuel);
	void SetFireResistance(int resistance);
	FORCEINLINE void SetWindVelocity(const FVector& velocity) { WindVelocity = velocity; };
	void SetStartingState(bool isObstacle = false);
	void SetCellSize(float size);
	void SetCellBoundsHidden(bool hidden);
	void SetTickInterval(float interval);

	void FindTerrainIntersection();
	void UseTerrainIntersection();
	void FindNeighbours();

	void TakeFireDamage(float damage); 
	void DamageNeighbourhingCells();
	
	FORCEINLINE void SetIgnition(bool isIgnitionPoint) { bIsIgnitionPoint = isIgnitionPoint; };
	UFUNCTION()
		void SetCellOnFire();
	UFUNCTION()
		void Reset();
	void ResetOnlyWhenBurnedOut();

	FORCEINLINE const FVector& GetIntersectionPoint() const { return TerrainIntersectionPoint; };
	FORCEINLINE const FRotator& GetIntersectionRotation() const { return TerrainIntersectionRotation; };

	FORCEINLINE void SetRegrowRate(float rate) { RegrowRate = rate; };
	FORCEINLINE void EnableRegrowth(bool enabled) { bCanRegrow = enabled; };

	FORCEINLINE void SetCanUseVFX(bool canUse) { bUseVFX = canUse; };

	//VFX are implemented in blueprints
	UFUNCTION(BlueprintImplementableEvent)
		void HandleFireVFX(bool activate);
	UFUNCTION(BlueprintImplementableEvent)
		void SetFireRotationAndLocation();

protected:
	void SetCellBurnedOut();
	void HandleFirstTickTimer();
	void HandleRegrowthTimer();

	UBoxComponent* CellBounds;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		UStaticMeshComponent* Trunk;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		UStaticMeshComponent* Crown;

	int MaxFuel;
	int Fuel;
	int MaxFireResistance;
	int FireResistance;
	FVector WindVelocity;

	float TickInterval;
	FTimerHandle FirstTickTimerHandle;

	float RegrowRate;
	bool bCanRegrow;
	FTimerHandle RegrowthTimerHandle;

	bool bUseVFX;

	bool bIsIgnitionPoint;
	TEnumAsByte<ECellState> StartingState;
	TEnumAsByte<ECellState> CellState;

	TArray<AChildCell*> NeighbourhingChildCells;
	TArray<FVector> NormalizedNeighbourDirections;

	UPROPERTY(BlueprintReadOnly)
		float CellSize;
	UPROPERTY(BlueprintReadOnly)	
		FVector TerrainIntersectionPoint;
	UPROPERTY(BlueprintReadOnly)
		FRotator TerrainIntersectionRotation;
};
