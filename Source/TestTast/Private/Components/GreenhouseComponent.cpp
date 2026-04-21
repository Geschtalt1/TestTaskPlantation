

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
	// Находим смещение курсора ОТНОСИТЕЛЬНО ЦЕНТРА сетки.
	FVector RelativeToCenter = WorldLocation - GridOrigin;

	const float Step = CellSpacing;

	// Вычисляем сырой индекс.
	float RawGridX = RelativeToCenter.X / Step;
	float RawGridY = RelativeToCenter.Y / Step;

	// 4. Находим индекс самой левой/нижней ячейки.
	// Если GridWidth=5, индексы: -2, -1, 0, 1, 2. Центр (0) получается при вычитании половины размера.
	const int32 GridIndexOffsetX = GridWidth / 2;
	const int32 GridIndexOffsetY = GridHeight / 2;

	// Округляем и применяем смещение, чтобы получить финальный индекс
	int32 GridX = FMath::RoundToInt(RawGridX) + GridIndexOffsetX;
	int32 GridY = FMath::RoundToInt(RawGridY) + GridIndexOffsetY;

	return FIntPoint(GridX, GridY);
}

FVector UGreenhouseComponent::GridToWorld(const FIntPoint& GridCoords) const
{
	if (!IsGridCoordsValid(GridCoords))
	{
		return FVector::ZeroVector;
	}

	const float Step = CellSpacing;

	// Находим смещение индекса ОТНОСИТЕЛЬНО ЦЕНТРА.
	// Если мы в ячейке с индексом 0 (самая левая), а всего ячеек 5,
	// то её смещение от центра: 0 - (5/2) = -2.5 шага.
	float OffsetFromCenterX = (GridCoords.X - (GridWidth / 2.0f)) * Step;
	float OffsetFromCenterY = (GridCoords.Y - (GridHeight / 2.0f)) * Step;

	// Находим центр ячейки в мировых координатах.
	// * GridOrigin - это центр всей сетки.
	// * OffsetFromCenter - это смещение от центра сетки до начала нужной ячейки.
	// * CellSize * 0.5f - это смещение от начала ячейки до её центра.
	float WorldX = GridOrigin.X + OffsetFromCenterX + (CellSize * 0.5f);
	float WorldY = GridOrigin.Y + OffsetFromCenterY + (CellSize * 0.5f);

	return FVector(WorldX, WorldY, GridOrigin.Z);
}

void UGreenhouseComponent::DrawDebugGreenhouseInfo(float Duration)
{
	GEngine->AddOnScreenDebugMessage(-1, Duration, FColor::White, "------------- End Greenhouse Component -------------");

	GEngine->AddOnScreenDebugMessage(-1, Duration, FColor::Yellow, FString::Printf(TEXT("- Cells Occupied: '%d' / '%d'"), CountOccupiedCells(), GetNumberOfPlantCells()));
	GEngine->AddOnScreenDebugMessage(-1, Duration, FColor::Yellow, FString::Printf(TEXT("- Plants Grown: '%d'"), FindPlantsByState(EPlantState::PS_Growing).Num()));
	GEngine->AddOnScreenDebugMessage(-1, Duration, FColor::Yellow, FString::Printf(TEXT("- Ready To Harvest: '%d'"), FindPlantsByState(EPlantState::PS_ReadyToHarvest).Num()));

	GEngine->AddOnScreenDebugMessage(-1, Duration, FColor::White, "---------------------------------------");

	FString CreatePlantationStatus = bIsCreatedPlantation ? "true" : "false";
	GEngine->AddOnScreenDebugMessage(-1, Duration, FColor::Cyan, FString::Printf(TEXT("- Is Created Plantation: '%s'"), *CreatePlantationStatus));

	GEngine->AddOnScreenDebugMessage(-1, Duration, FColor::White, "------------- Begin Greenhouse Component -------------");
}

void UGreenhouseComponent::DrawDebugGreenhousePlantCells(float Duration)
{
	const FVector HalfCellSize = FVector(CellSize * 0.5f);
	const FVector HalfGridSize = FVector((GridWidth - 1) * CellSpacing * 0.5f, (GridHeight - 1) * CellSpacing * 0.5f, 0.0f);

    for (int32 y = 0; y < GridHeight; ++y)
    {
        for (int32 x = 0; x < GridWidth; ++x)
        {
			FPlantCell PlantCell;
			int32 Index = y * GridWidth + x;

			FindPlantByIndexCell(Index, PlantCell);
			FLinearColor ColorCell;

			if (PlantCell.State == EPlantState::PS_Empty)
			{
				ColorCell = FLinearColor::Red;
			}
			else if (PlantCell.State == EPlantState::PS_Growing)
			{
				ColorCell = FLinearColor::Green;
			}
			else if (PlantCell.State == EPlantState::PS_ReadyToHarvest)
			{
				ColorCell = FLinearColor::Yellow;
			}
			else
			{
				ColorCell = FLinearColor::Black;
			}

			// Вычисляем центр ячейки.
			// GridOrigin - центр всей сетки.
			// (x - (GridWidth-1)/2) - смещение относительно центра по X.
			float OffsetX = (x - (GridWidth - 1) / 2.0f) * CellSpacing;
			float OffsetY = (y - (GridHeight - 1) / 2.0f) * CellSpacing;

			FVector CellLocation = GridOrigin + FVector(OffsetX, OffsetY, 0.0f);

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
