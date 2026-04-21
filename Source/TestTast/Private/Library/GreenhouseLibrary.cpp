

#include "Library/GreenhouseLibrary.h"
#include "Components/GreenhouseComponent.h"

bool UGreenhouseLibrary::IsValidPlantCell(const FPlantCell& PlantCell)
{
	return PlantCell.IsValid();
}

bool UGreenhouseLibrary::IsOccupiedPlantCell(const FPlantCell& PlantCell)
{
	return PlantCell.IsOccupied();
}

int32 UGreenhouseLibrary::GetIndexPlantCell(const FPlantCell& PlantCell)
{
	if (PlantCell.IsValid())
	{
		return PlantCell.OwningComponent->GetPlantCellIndex(PlantCell);
	}
	return -1;
}