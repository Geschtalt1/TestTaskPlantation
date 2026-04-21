
#pragma once

#include "GameplayTagContainer.h"

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PlantData.generated.h"

USTRUCT(BlueprintType)
struct FPlantGrowthResourceConfig
{
    GENERATED_BODY()

public:
    FPlantGrowthResourceConfig()
    {}

    FPlantGrowthResourceConfig(FGameplayTag InResourceTag)
        : ResourceTag(InResourceTag)
    {}

public:
    /**
     * @brief Тег ресурса (например, вода, свет).
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTag ResourceTag = FGameplayTag();

    /**
     * @brief Требуемое значение ресурса для поддержания оптимального роста.
     *        Это базовая величина, относительно которой рассчитываются множители и пороги.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float BaseValue = 10.0f;

    /**
     * @brief Начальное значение этого ресурса при добавление в ячейку.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
    float InitialValue = 0.0f;

    /**
     * @brief Минимальное относительное значение ресурса, ниже которого рост полностью останавливается.
     *        Выражается как доля от BaseValue (например, 0.1f означает 10% от BaseValue).
     *        Если текущий уровень ресурса ниже этого порога, рост прекращается.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "1"))
    float MinValueForGrowth = 0.1f; // Например, 10%

    /**
     * @brief Минимальное относительное значение ресурса, ниже которого ячейка считается критически заблокированной.
     *        Выражается как доля от BaseValue (например, 0.2f означает 20% от BaseValue).
     *        Если текущий уровень ресурса ниже этого порога, ячейка переходит в состояние Blocked.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "1"))
    float CriticalThreshold = 0.2f; // Например, 20%

    /**
     * @brief Скорость потребления данного ресурса за один тик симуляции.
     *        Выражается как доля от BaseValue (например, 0.02f означает, что за тик потребляется 2% от BaseValue).
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
    float ConsumptionRate = 0.02f; // Например, 2% от BaseValue за тик

public:
    bool operator==(const FPlantGrowthResourceConfig& Other) const
    {
        return ResourceTag == Other.ResourceTag;
    }
};

/**
 * 
 */
UCLASS()
class TESTTAST_API UPlantData : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
    /** @brief Идентификатор растения. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
    FName PlantName = FName();

    /** @brief Название растения отоброжаемого в UI. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Common")
    FText PlantDisplayName;

    /** @brief Общий прогресс роста для созревания. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "100"), Category = "General Growth")
    float GrowthStages = 100.0f;

    /** @brief Начальный прогресс роста. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "100"), Category = "General Growth")
    float InitialGrowthProgress = 0.0f;

    /**
     * @brief Множитель скорости роста при идеальных условиях (все ресурсы на уровне BaseValue или выше).
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0"), Category = "General Growth|Coefficients")
    float IdealGrowthMultiplier = 1.5f;

    /**
     * @brief Множитель скорости роста при плохих (но жизнеспособных) условиях.
     *        Используется, когда ресурсы удовлетворяют минимальному порогу для роста, но не являются идеальными.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0"), Category = "General Growth|Coefficients")
    float PoorGrowthMultiplier = 0.5f;

    /** @brief Необходимые ресурсы для полного роста. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configs")
    TArray<FPlantGrowthResourceConfig> ResourceConfigs;
};
