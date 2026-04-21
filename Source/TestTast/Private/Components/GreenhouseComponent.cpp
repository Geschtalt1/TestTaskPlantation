

#include "Components/GreenhouseComponent.h"

#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"

UGreenhouseComponent::UGreenhouseComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UGreenhouseComponent::BeginPlay()
{
	Super::BeginPlay();
	
	CreatePlantation();
}

void UGreenhouseComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bShowDebugGreenhouseInfo)
	{
		DrawDebugGreenhouseInfo(DeltaTime);
		DrawDebugGreenhousePlantCells(DeltaTime);
	}
}

void UGreenhouseComponent::CreatePlantation()
{
	if (bIsCreatedPlantation) {
		return;
	}

	PlantCells.Empty();

	const int32 CountCells = GridHeight * GridWidth;
	for (int32 i = 0; i < CountCells; i++)
	{
		PlantCells.Add(MakePlantCell());
	}

	bIsCreatedPlantation = true;
	OnPlantationCreated.Broadcast();
}

void UGreenhouseComponent::ClearPlantation()
{
	if (!bIsCreatedPlantation) {
		return;
	}

	PlantCells.Empty();
	bIsCreatedPlantation = false;

	OnPlantationCleared.Broadcast();
}

void UGreenhouseComponent::UpdateSimulation(float DeltaTime)
{
	for (auto& Cell : PlantCells)
	{
		if (!Cell.IsOccupied()) {
			continue;
		}

		Cell.Update(DeltaTime);
	}

	OnSimulationUpdated.Broadcast();
}

bool UGreenhouseComponent::PlantSeed(UPlantData* PlantData, int32 IndexCell)
{
	if (!PlantData) {
		return false;
	}

	if (!HasPlantCell(IndexCell) || IsOccupiedPlantCell(IndexCell))
	{
		return false;
	}

	PlantCells[IndexCell].SetPlant(PlantData);
	OnPlantSeeded.Broadcast(PlantData, IndexCell);

	return true;
}

bool UGreenhouseComponent::RemovePlantAt(int32 IndexCell)
{
	if (IsOccupiedPlantCell(IndexCell))
	{
		PlantCells[IndexCell].Clear();
		OnPlantationPlantRemoved.Broadcast(IndexCell);

		return true;
	}
	return false;
}

bool UGreenhouseComponent::SetResourceValueToPlantCell(const FGameplayTag InTagResource, float NewValue, int32 IndexCell)
{
	if (InTagResource.IsValid() && IsOccupiedPlantCell(IndexCell))
	{
		return PlantCells[IndexCell].SetResourceValue(InTagResource, NewValue);
	}
	return false;
}

bool UGreenhouseComponent::FindPlantByIndexCell(int32 IndexCell, FPlantCell& FoundPlant) const
{
	if (HasPlantCell(IndexCell))
	{
		FoundPlant = PlantCells[IndexCell];
		return true;
	}
	return false;
}

bool UGreenhouseComponent::FindPlantCellByCoords(const FIntPoint& GridCoords, FPlantCell& FoundPlant) const
{
	// Проверяем, что координаты находятся в пределах сетки.
	if (!IsGridCoordsValid(GridCoords))
	{
		return false;
	}

	const int32 Index = GridCoords.Y * GridWidth + GridCoords.X;
	if (!HasPlantCell(Index))
	{
		return false;
	}

	FoundPlant = PlantCells[Index];
	return true;
}

TArray<FPlantCell> UGreenhouseComponent::FindPlantsByState(const EPlantState InState) const
{
	TArray<FPlantCell> Arr;
	for (auto& Cell : PlantCells)
	{
		if (Cell.State == InState)
		{
			Arr.Add(Cell);
		}
	}
	return Arr;
}

int32 UGreenhouseComponent::FindFirstFreePlantCellIndex() const
{
	for (int32 i = 0; i < PlantCells.Num(); i++)
	{
		if (!PlantCells[i].IsOccupied())
		{
			return i;
		}
	}
	return -1;
}

int32 UGreenhouseComponent::CountPlants(UPlantData* PlantData) const
{
	if (!PlantData)
	{
		return 0;
	}

	int32 Count = 0;
	for (auto& Cell : PlantCells)
	{
		if (Cell.Data == PlantData)
		{
			Count++;
		}
	}
	return Count;
}

int32 UGreenhouseComponent::CountOccupiedCells() const
{
	int32 Count = 0;
	for (auto& Cell : PlantCells)
	{
		if (Cell.IsOccupied())
		{
			Count++;
		}
	}
	return Count;
}

FPlantCell UGreenhouseComponent::MakePlantCell()
{
	FPlantCell NewPlantCell = FPlantCell(this);
	NewPlantCell.Clear();

	return NewPlantCell;
}

FIntPoint UGreenhouseComponent::WorldToGrid(const FVector& WorldLocation) const
{
	// Используем GridOrigin как начало сетки.
	FVector RelativeLocation = WorldLocation - GridOrigin;

	// Полный шаг от начала одной ячейки до начала следующей.
	const float Step = CellSize + CellSpacing;

	int32 GridX = FMath::RoundToInt(RelativeLocation.X / Step);
	int32 GridY = FMath::RoundToInt(RelativeLocation.Y / Step);

	return FIntPoint(GridX, GridY);
}

FVector UGreenhouseComponent::GridToWorld(const FIntPoint& GridCoords) const
{
	if (!IsGridCoordsValid(GridCoords))
	{
		return FVector::ZeroVector;
	}

	const float Step = CellSize + CellSpacing;

	// Находим начало ячейки и смещаемся к её центру
	float RelativeX = GridCoords.X * Step + (CellSize * 0.5f);
	float RelativeY = GridCoords.Y * Step + (CellSize * 0.5f);

	FVector WorldLocation = GridOrigin + FVector(RelativeX, RelativeY, 0.0f);
	return WorldLocation;
}

void UGreenhouseComponent::DrawDebugGreenhouseInfo(float Duration)
{
	GEngine->AddOnScreenDebugMessage(-1, Duration, FColor::White, "------------- End Greenhouse Component -------------");

	GEngine->AddOnScreenDebugMessage(-1, Duration, FColor::Yellow, FString::Printf(TEXT("- Cells Occupied: '%d' / '%d'"), CountOccupiedCells(), GetNumberOfPlantCells()));
	GEngine->AddOnScreenDebugMessage(-1, Duration, FColor::Yellow, FString::Printf(TEXT("- Plants Grown: '%d'"), FindPlantsByState(EPlantState::PS_Growing).Num()));

	GEngine->AddOnScreenDebugMessage(-1, Duration, FColor::White, "---------------------------------------");

	FString CreatePlantationStatus = bIsCreatedPlantation ? "true" : "false";
	GEngine->AddOnScreenDebugMessage(-1, Duration, FColor::Cyan, FString::Printf(TEXT("- Is Created Plantation: '%s'"), *CreatePlantationStatus));

	GEngine->AddOnScreenDebugMessage(-1, Duration, FColor::White, "------------- Begin Greenhouse Component -------------");
}

void UGreenhouseComponent::DrawDebugGreenhousePlantCells(float Duration)
{
	const FVector OwnerLocation = GridOrigin;

    for (int32 y = 0; y < GridHeight; ++y)
    {
        for (int32 x = 0; x < GridWidth; ++x)
        {
			FPlantCell PlantCell;
			int32 Index = y * GridWidth + x;

			FindPlantByIndexCell(Index, PlantCell);
			FLinearColor ColorCell = PlantCell.IsOccupied() ? FLinearColor::Green : FLinearColor::Red;

			// Примерный размер ячейки.
			FVector CellLocation = OwnerLocation + FVector(x * CellSpacing, y * CellSpacing, 0.0f);

			// Draw Debug Box.
			UKismetSystemLibrary::DrawDebugBox(
				this,
				CellLocation,
				FVector(CellSize),
				ColorCell,
				FRotator(),
				Duration,
				2.5f
			);
        }
    }
}
