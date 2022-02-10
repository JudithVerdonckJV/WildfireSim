#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Observer.generated.h"

class USpringArmComponent;
class UCameraComponent;
class USceneComponent;
class APlayerController;
class AChildCell;
class AParentCell;
class AGridGenerator;

UCLASS()
class WILDFIRESIM_API AObserver : public APawn
{
	GENERATED_BODY()

public:
	AObserver();

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintCallable)
		bool StartFireSimulation();
	UFUNCTION(BlueprintCallable)
		void StopFireSimulationAndReset();
	UFUNCTION(BlueprintCallable)
		void SetParentCellSize(float size);
	UFUNCTION(BlueprintCallable)
		void SetChildCellNumber(int nr);
	UFUNCTION(BlueprintCallable)
		void RegenerateGrid();
	UFUNCTION(BlueprintCallable)
		void SetTickRateOnGrid(float tickRate);
	UFUNCTION(BlueprintCallable)
		void RegrowBurnedTrees();
	UFUNCTION(BlueprintCallable)
		void SelectIgnitionOrCell(bool ignition) { bPickIgnition = ignition; };
	UFUNCTION(BlueprintCallable)
		void SetFuelInSelectedCell(int fuel);
	UFUNCTION(BlueprintCallable)
		void SetGeneralFuel(int fuel);
	UFUNCTION(BlueprintCallable)
		void SetResistanceInSelectedCell(int resistance);
	UFUNCTION(BlueprintCallable)
		void SetGeneralResistance(int resistance);
	UFUNCTION(BlueprintCallable)
		void SetWindInSelectedCell(const FVector& windVel);
	UFUNCTION(BlueprintCallable)
		void SetGeneralWind(const FVector& windVel);

	UFUNCTION(BlueprintCallable)
		void SetRegrowRate(float rate);
	UFUNCTION(BlueprintCallable)
		void EnableRegrowth(bool enabled);

	UFUNCTION(BlueprintCallable)
		void SetCanUseVFX(bool canUse);

protected:
	virtual void BeginPlay() override;
	void SelectCell();

	UFUNCTION(BlueprintImplementableEvent)
		void SetIgnitionPoint_VFX(const FVector& location, const FRotator& rotation);
	UFUNCTION(BlueprintImplementableEvent)
		void SetSelectedCellVisual(const FVector& location, const FVector& halfextents, const FVector& windDirection);
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
		void UpdateWindVelocityVisual(const FVector& windVel);

	bool bPickIgnition;

	UPROPERTY(EditAnywhere)
		USceneComponent* Root;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		USpringArmComponent* SpringArm;
	UPROPERTY(EditAnywhere)
		UCameraComponent* Camera;

	UPROPERTY(EditAnywhere)
		AGridGenerator* GridGenerator;

private:
	APlayerController* PlayerController;
	AChildCell* IgnitionCell;
	AParentCell* SelectedCell;
	float TraceDistance;
};
