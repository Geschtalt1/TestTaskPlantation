
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GreenhouseLibrary.generated.h"

struct FPlantCell;

/**
 * 
 */
UCLASS()
class TESTTAST_API UGreenhouseLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Greenhouse System")
	static bool IsValidPlantCell(const FPlantCell& PlantCell);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Greenhouse System")
	static bool IsOccupiedPlantCell(const FPlantCell& PlantCell);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Greenhouse System")
	static int32 GetIndexPlantCell(const FPlantCell& PlantCell);
};
