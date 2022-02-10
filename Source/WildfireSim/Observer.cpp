#include "Observer.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

#include "Cells/ChildCell.h"
#include "Cells/ParentCell.h"
#include "Managers/GridGenerator.h"

AObserver::AObserver()
	: bPickIgnition{ true }
	, Root{}
	, SpringArm{}
	, Camera{}
	, GridGenerator{}
	, PlayerController{}
	, IgnitionCell{}
	, TraceDistance{ 10000.f }
{
 	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>("Root");
	SpringArm = CreateDefaultSubobject<USpringArmComponent>("SpringArm");
	Camera = CreateDefaultSubobject<UCameraComponent>("Camera");

	SetRootComponent(Root);
	SpringArm->SetupAttachment(Root);
	Camera->SetupAttachment(SpringArm);

	SpringArm->TargetArmLength = 5000.f;
	SpringArm->bDoCollisionTest = false;
}

void AObserver::BeginPlay()
{
	Super::BeginPlay();

	PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	PlayerController->bShowMouseCursor = true;
	
	AActor* gridGen = UGameplayStatics::GetActorOfClass(GetWorld(), AGridGenerator::StaticClass());
	GridGenerator = Cast<AGridGenerator>(gridGen);
}

void AObserver::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("Select", EInputEvent::IE_Pressed, this, &AObserver::SelectCell);
}

void AObserver::SelectCell()
{
	float mouseX{};
	float mouseY{};
	FVector worldPos{};
	FVector direction{};

	PlayerController->GetMousePosition(mouseX, mouseY);
	UGameplayStatics::DeprojectScreenToWorld(PlayerController, { mouseX, mouseY }, worldPos, direction);

	FHitResult hitResult{};
	GetWorld()->LineTraceSingleByChannel(hitResult, worldPos, worldPos + direction * TraceDistance, ECollisionChannel::ECC_Visibility);

	if (hitResult.bBlockingHit)
	{
		const FVector pickedLocation{ hitResult.ImpactPoint };
		AParentCell* pickedParentCell = GridGenerator->FindPickedParentCell(pickedLocation);

		if (bPickIgnition)
		{
			if (IgnitionCell)
			{
				IgnitionCell->SetIgnition(false);
			}
			IgnitionCell = pickedParentCell->GetPickedChildCell(pickedLocation);
			IgnitionCell->SetIgnition(true);
			SetIgnitionPoint_VFX(IgnitionCell->GetIntersectionPoint(), IgnitionCell->GetIntersectionRotation());
		}
		else
		{
			SelectedCell = pickedParentCell;
			SetSelectedCellVisual(SelectedCell->GetActorLocation(),
								  SelectedCell->GetBounds()->GetScaledBoxExtent(),
								  SelectedCell->GetWindVelocity());
		}
	}
}

bool AObserver::StartFireSimulation()
{
	if (!IgnitionCell)
	{
		return false;
	}
	
	IgnitionCell->SetCellOnFire();
	return true;
}

void AObserver::StopFireSimulationAndReset()
{
	GridGenerator->StopFireSimulation();
}

void AObserver::SetParentCellSize(float size)
{
	GridGenerator->SetParentCellSize(size);
}

void AObserver::SetChildCellNumber(int nr)
{
	GridGenerator->SetChildCellNumber(nr);
}

void AObserver::RegenerateGrid()
{
	GridGenerator->GenerateGrids();
}

void AObserver::SetTickRateOnGrid(float tickRate)
{
	GridGenerator->SetTickRateOnGrid(tickRate);
}

void AObserver::RegrowBurnedTrees()
{
	GridGenerator->RegrowBurnedTrees();
}

void AObserver::SetFuelInSelectedCell(int fuel)
{
	if (!SelectedCell) return;

	SelectedCell->SetBaseFuel(fuel);
}

void AObserver::SetGeneralFuel(int fuel)
{
	GridGenerator->SetFuelOnGrid(fuel);
}

void AObserver::SetResistanceInSelectedCell(int resistance)
{
	if (!SelectedCell) return;

	SelectedCell->SetBaseFireResistance(resistance);
}

void AObserver::SetGeneralResistance(int resistance)
{
	GridGenerator->SetResistanceOnGrid(resistance);
}

void AObserver::SetWindInSelectedCell(const FVector& windVel)
{
	if (!SelectedCell) return;

	SelectedCell->SetWindVelocity(windVel);
}

void AObserver::SetGeneralWind(const FVector& windVel)
{
	GridGenerator->SetWindVelocityOnGrid(windVel);
}

void AObserver::SetRegrowRate(float rate)
{
	GridGenerator->SetRegrowRate(rate);
}

void AObserver::EnableRegrowth(bool enabled)
{
	GridGenerator->EnableRegrowth(enabled);
}

void AObserver::SetCanUseVFX(bool canUse)
{
	GridGenerator->SetCanUseVFX(canUse);
}