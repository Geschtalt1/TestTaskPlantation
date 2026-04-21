
#pragma once

#include "Data/PlantData.h"
#include "GameplayTagContainer.h"

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GreenhouseComponent.generated.h"

UENUM(BlueprintType)
enum class EPlantState : uint8
{
	PS_Empty             UMETA(DisplayName = "Empty"),
	PS_Growing           UMETA(DisplayName = "Growing"),
	PS_ReadyToHarvest    UMETA(DisplayName = "Ready to Harvest"),
	PS_Blocked           UMETA(DisplayName = "Blocked")
};

USTRUCT(BlueprintType)
struct FPlantCell
{
	GENERATED_BODY()

public:
	FPlantCell()
	{}

	FPlantCell(UGreenhouseComponent* InOwningComponent)
		: OwningComponent(InOwningComponent)
	{ }

	FPlantCell(UPlantData* InData, UGreenhouseComponent* InOwningComponent)
		: Data(InData),
		  OwningComponent(InOwningComponent)
	{
		Reset();
	}

public:
	bool operator==(const FPlantCell& Other) const
	{
		return Id == Other.Id;
	}

public:
	void SetPlant(UPlantData* InData)
	{
		if (!InData)
		{
			return;
		}

		Data = InData;
		Reset();
	}

bool SetResourceValue(const FGameplayTag& InResTag, float NewValue)
	{
		if (Resources.Contains(InResTag))
		{
			// Обновляем значение ресурса, если такой ресурс есть у растения.
			Resources[InResTag] = FMath::Max(0.0f, NewValue);
			return true;
		}
		return false;
	}

	void SetGrowthProgress(float NewProgress)
	{
		GrowthProgress = FMath::Max(0.0f, NewProgress);
	}

	void Reset()
	{
		if (Data)
		{
			LoadResources();
			SetGrowthProgress(Data->InitialGrowthProgress);

			State = EPlantState::PS_Blocked;
		}
	}

	void Update(float DeltaTime)
	{
		if ((State == EPlantState::PS_Empty) || (State == EPlantState::PS_ReadyToHarvest) || (!Data))
		{
			return;
		}

		// --- Этап 1: Потребление ресурсов ---
		for (auto& Pair : Resources)
		{
			FPlantGrowthResourceConfig ResConfig;
			FindResourceConfig(Pair.Key, ResConfig);

			const float ConsumpAmount = ResConfig.BaseValue * ResConfig.ConsumptionRate;
			const float NewResValue = Pair.Value - ConsumpAmount;

			// Потребляем ресурс.
			SetResourceValue(Pair.Key, NewResValue); 
		}

		// --- Этап 2: Обработка роста ---
		bool bCanGrow = true; // Можно ли вообще расти?
		bool bIsBlocked = false; // Критическая ситуация?

		float CombinedResourceSatisfaction = 0.0f; // Для расчета множителя роста.

		for (const auto& Pair : Resources)
		{
			const float CurrentResValue = Pair.Value;

			FPlantGrowthResourceConfig ResConfig;
			FindResourceConfig(Pair.Key, ResConfig);

			// Рассчитываем относительные уровни.
			const float MinGrowthValue = ResConfig.BaseValue * ResConfig.MinValueForGrowth;
			const float CriticalValue = ResConfig.BaseValue * ResConfig.CriticalThreshold;

			// Проверяем, можем ли мы расти.
			if (CurrentResValue < MinGrowthValue)
			{
				bCanGrow = false;
			}

			// Проверяем, критическая ли ситуация.
			if (CurrentResValue < CriticalValue)
			{
				bIsBlocked = true;
			}

			// Рассчитываем удовлетворенность ресурсом для множителя роста.
			const float ResourceSatisfaction = FMath::Clamp(CurrentResValue / ResConfig.BaseValue, 0.0f, 1.0f);
			CombinedResourceSatisfaction += ResourceSatisfaction;
		}

		// --- Применение множителя к прогрессу роста ---
		if (bCanGrow)
		{
			// --- Расчет финального множителя роста ---
			float FinalGrowthMultiplier = 1.0f;

			// Средняя удовлетворенность ресурсами.
			float AverageResourceSatisfaction = CombinedResourceSatisfaction / Resources.Num();

			FinalGrowthMultiplier = FMath::GetMappedRangeValueClamped(
				FVector2D(0.0f, 1.0f),                                                  // Вход средняя удовлетворенность ресурсами
				FVector2D(Data->PoorGrowthMultiplier, Data->IdealGrowthMultiplier),     // Множитель роста
				AverageResourceSatisfaction
			);

			// Если GrowthStages это целевое значение (например, 100.0f), которое нужно достичь.
			// То BaseGrowthPerSecond, будет означать сколько единиц прогресса растение набирает за одну секунду при идеальных условиях.
			const float BaseGrowthPerSecond = 100.0f / Data->GrowthStages;

			// Это количество прогресса, которое растение наберет за текущий маленький промежуток времени DeltaTime, с учетом множителя роста.
			const float GrowthAmountThisTick = (BaseGrowthPerSecond * DeltaTime) * FinalGrowthMultiplier;

			SetGrowthProgress(GrowthProgress += GrowthAmountThisTick);
		}

		// --- Обновляем состояние ячейки ---
		if (bIsBlocked)
		{
			State = EPlantState::PS_Blocked;
		}
		// Проверяем, достиг ли прогресс целевого значения.
		else if (GrowthProgress >= Data->GrowthStages)
		{
			State = EPlantState::PS_ReadyToHarvest;
		}
		else if (bCanGrow) // Растет, но еще не созрел.
		{
			State = EPlantState::PS_Growing;
		}
	}

	void Clear()
	{
		Data = nullptr;
		Resources.Empty();
		GrowthProgress = 0.0f;
		State = EPlantState::PS_Empty;
	}

	bool FindResourceConfig(const FGameplayTag& InResTag, FPlantGrowthResourceConfig& OutConfig) const
	{
		if (Data)
		{
			const FPlantGrowthResourceConfig ConfigTemp = FPlantGrowthResourceConfig(InResTag);
			const int32 Index = Data->ResourceConfigs.Find(ConfigTemp);

			if (Index == -1) {
				return false;
			}

			OutConfig = Data->ResourceConfigs[Index];
			return true;
		}
		return false;
	}

private:
	void LoadResources()
	{
		if (Data)
		{
			Resources.Empty();
			for (auto& Config : Data->ResourceConfigs)
			{
				Resources.Add(Config.ResourceTag, Config.InitialValue);
			}
		}
	}

public:
	FORCEINLINE FGuid GetId() const { return Id; }
	FORCEINLINE bool IsValid() const { return OwningComponent != nullptr; }
	FORCEINLINE bool IsOccupied() const { return Data != nullptr; }

public:
	/** @brief Данные конкретного типа растения. */
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UPlantData> Data = nullptr;

	/** @brief Текущий уровень роста. */
	UPROPERTY(BlueprintReadOnly)
	float GrowthProgress = 0.0f;

	/** @brief Текущии значения ресурсов. */
	UPROPERTY(BlueprintReadOnly)
	TMap<FGameplayTag, float> Resources;

	/** @brief Текущий уровень воды. */
	UPROPERTY(BlueprintReadOnly)
	EPlantState State = EPlantState::PS_Empty;

	/** @brief Плантация к которому принадлежит ячейка. */
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UGreenhouseComponent> OwningComponent = nullptr;

private:
	FGuid Id = FGuid::NewGuid();
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlantationCreated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlantationCleared);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSimulationUpdated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlantationPlantRemoved, int32, IndexCell);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlantSeeded, UPlantData*, PlantData, int32, IndexCell);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TESTTAST_API UGreenhouseComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UGreenhouseComponent();

public:
	/**
	 * @brief Инициализирует сетку теплицы, создавая пустые ячейки или устанавливая начальное состояние.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility")
	void CreatePlantation();

	/**
	 * @brief Удаляет сетку теплицы.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility")
	void ClearPlantation();

	/**
	 * @brief Обновляет состояние всей системы теплицы (симуляцию).
	 *
	 * @param DeltaTime Время, прошедшее с момента последнего кадра (в секундах).
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility")
	void UpdateSimulation(float DeltaTime);

	/**
	 * @brief Устанавливает новое значение для определенного ресурса в указанной ячейке растения.
	 *
	 * @param InTagResource Тег GameplayTag, идентифицирующий тип ресурса.
	 * @param NewValue Новое значение, которое нужно установить для указанного ресурса.
	 * @param IndexCell Индекс ячейки растения, в которой нужно обновить ресурс.
	 * 
	 * @return true, если значение ресурса было успешно установлено, false в противном случае (например, если тег некорректен или индекса нет).
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility")
	bool SetResourceValueToPlantCell(const FGameplayTag InTagResource, float NewValue, int32 IndexCell);

	/**
	 * @brief Садит растение в указанную ячейку сетки.
	 *
	 * @param PlantData Указатель на данные растения, которое нужно посадить.
	 * @param IndexCell Индекс ячейки в одномерном массиве, куда следует посадить растение.
	 * 
	 * @return True, если посадка прошла успешно, иначе false.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility")
	bool PlantSeed(UPlantData* PlantData, int32 IndexCell);

	/**
	 * @brief Удаляет растение из указанной ячейки.
	 *
	 * @param IndexCell Индекс ячейки, из которой нужно удалить растение.
	 * @return True, если растение было успешно удалено (т.е. ячейка была занята), иначе false.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility")
	bool RemovePlantAt(int32 IndexCell);

	/**
	 * @brief Находит и возвращает информацию о растении, находящемся в указанной ячейке.
	 *
	 * @param IndexCell Индекс ячейки, в которой ищется растение.
	 * @param FoundPlant Структура, в которую будут скопированы найденные данные о растении.
	 * 
	 * @return True, если растение было найдено в указанной ячейке, иначе false.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility")
	bool FindPlantByIndexCell(int32 IndexCell, FPlantCell& FoundPlant) const;

	/**
	 * @brief Выполняет поиск данных о растении по заданным координатам сетки.
	 *
	 * @param GridCoords Координаты ячейки, в которой нужно выполнить поиск.
	 * @param FoundPlant Ссылка на объект, куда будут записаны данные о найденном растении.
	 * 
	 * @return true, если ячейка занята и данные успешно записаны в FoundPlant; иначе false.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility")
	bool FindPlantCellByCoords(const FIntPoint& GridCoords, FPlantCell& FoundPlant) const;

	/**
	 * @brief Выполняет поиск всех ячеек сетки, в которых находятся растения с указанным состоянием.
	 *
	 * @param InState Состояние растения, по которому будет производиться фильтрация.
	 * @return TArray<FPlantCell> Массив, содержащий данные о всех найденных ячейках, соответствующих заданному состоянию.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility")
	TArray<FPlantCell> FindPlantsByState(const EPlantState InState) const;

	/**
	 * @brief Находит индекс первой свободной (пустой) ячейки в сетке теплицы.
	 * @return Индекс первой свободной ячейки, или -1, если все ячейки заняты.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility")
	int32 FindFirstFreePlantCellIndex() const;

	/**
	 * @brief Подсчитывает количество растений определенного типа, находящихся в теплице.
	 *
	 * @param PlantData Указатель на данные растения, количество которого нужно подсчитать.
	 * @return Количество растений данного типа, найденных в теплице.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility")
	int32 CountPlants(UPlantData* PlantData) const;

	/**
	 * @brief Подсчитывает количество ячеек в теплице, которые в данный момент заняты растениями.
	 * @return Общее количество занятых ячеек.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility")
	int32 CountOccupiedCells() const;

	/**
	 * @brief Преобразует точку в мировых координатах (World Space) в целочисленные координаты сетки (Grid Coordinates).
	 * Координаты отсчитываются от центра сетки, где находится GridOrigin.
	 *
	 * @param WorldLocation Точка в мировом пространстве, которую необходимо преобразовать.
	 * @return FIntPoint Координаты ячейки сетки. Если точка находится за пределами сетки, координаты будут отрицательными.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility")
	FIntPoint WorldToGrid(const FVector& WorldLocation) const;

	/**
	 * @brief Преобразует целочисленные координаты сетки в точку мирового пространства (World Space).
	 * Возвращает позицию центра указанной ячейки.
	 *
	 * @param GridCoords Координаты ячейки на сетке (X, Y).
	 * @return FVector Точка в мировом пространстве, соответствующая центру ячейки.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility")
	FVector GridToWorld(const FIntPoint& GridCoords) const;

public:
	FPlantCell MakePlantCell();

public:
	void DrawDebugGreenhouseInfo(float Duration = 2.0f);
	void DrawDebugGreenhousePlantCells(float Duration = 2.0f);

public:
	/**
	 * @brief Проверяет, находятся ли переданные координаты в допустимых границах сетки.
	 *
	 * @param GridCoords Координаты для проверки.
	 * @return true, если координаты находятся внутри сетки; иначе false.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility")
	FORCEINLINE bool IsGridCoordsValid(const FIntPoint& GridCoords) const
	{
		return GridCoords.X >= 0 && GridCoords.X < GridWidth &&
			GridCoords.Y >= 0 && GridCoords.Y < GridHeight;
	}

	/**
	 * @brief Проверяет, существует ли запись для указанной ячейки в списке посадок.
	 *
	 * @param IndexCell Индекс ячейки, существование записи для которой проверяется.
	 * @return True, если запись для ячейки существует в списке PlantCells, иначе false.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility")
	FORCEINLINE bool HasPlantCell(int32 IndexCell) const { return PlantCells.IsValidIndex(IndexCell); }

	/**
	 * @brief Проверяет, является ли указанная ячейка корректной и содержит ли растение.
	 *
	 * @param IndexCell Индекс ячейки, валидность которой проверяется.
	 * @return True, если ячейка существует в списке и содержит валидное растение, иначе false.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility")
	FORCEINLINE bool IsOccupiedPlantCell(int32 IndexCell) const
	{
		return HasPlantCell(IndexCell) ? PlantCells[IndexCell].IsOccupied() : false;
	}

	/**
	 * @brief Возвращает копию всех текущих ячеек для растений в теплице.
	 * @return Массив структур FPlantCell, содержащий данные обо всех текущих ячейках.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility")
	FORCEINLINE TArray<FPlantCell> GetPlantCells() const { return PlantCells; }

	/**
	 * @brief Возвращает общее количество текущих посадок (растений) в теплице.
	 * @return Количество текущих посадок.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility")
	FORCEINLINE int32 GetNumberOfPlantCells() const { return PlantCells.Num(); }

	/**
	 * @brief Флаг, указывающий была ли создана и инициализирована плантация.
	 * @return True если была инициализирована, False если нет.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility")
	FORCEINLINE bool IsCreatedPlantation() const { return bIsCreatedPlantation; }

	/**
	 * @brief Возращает индекс ячейки в массиве в котором находится растение. 
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility")
	FORCEINLINE int32 GetPlantCellIndex(const FPlantCell& InPlantCell) const
	{
		return PlantCells.Find(InPlantCell);
	}

public:
	/**
	 * @brief Определяет ширину сетки плантации, на которой будут располагаться растения.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1"), Category = "Config")
	int32 GridWidth = 20;

	/**
	 * @brief Определяет высоту сетки плантации, на которой будут располагаться растения.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1"), Category = "Config")
	int32 GridHeight = 20;

public:
	/**
	 * @brief Точка в мировом пространстве, которая является центром всей сетки.
	 * Все расчеты и отрисовка ячеек производятся относительно этой точки.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Setting")
	FVector GridOrigin;

	/**
	 * @brief Физический размер одной ячейки.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Setting")
	float CellSize = 15.0f;

	/**
	 * @brief Расстояние между центрами соседних ячеек сетки.
	 * Определяет плотность посадки растений и общий размер теплицы.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Setting")
	float CellSpacing = 100.0f;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bShowDebugGreenhouseInfo = false;

public:
	/**
	 * @brief Делегат, вызываемый после успешного создания или инициализации всей теплицы.
	 */
	UPROPERTY(BlueprintAssignable)
	FOnPlantationCreated OnPlantationCreated;

	/**
	 * @brief Делегат, вызываемый после удаление всех слотов в теплицы.
	 */
	UPROPERTY(BlueprintAssignable)
	FOnPlantationCleared OnPlantationCleared;

	/**
	 * @brief Делегат, вызываемый после удаления растения из ячейки.
	 */
	UPROPERTY(BlueprintAssignable)
	FOnPlantationPlantRemoved OnPlantationPlantRemoved;

	/**
	 * @brief Делегат, вызываемый после успешной посадки семени в ячейку.
	 */
	UPROPERTY(BlueprintAssignable)
	FOnPlantSeeded OnPlantSeeded;

	/****/
	UPROPERTY(BlueprintAssignable)
	FOnSimulationUpdated OnSimulationUpdated;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	/**
	 * @brief Флаг, указывающий была ли создана и инициализирована плантация.Массив, хранящий информацию о каждой отдельной ячейке плантации.
	 */
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Private")
	bool bIsCreatedPlantation = false;

	/**
	 * @brief Массив, хранящий информацию о каждой отдельной ячейке плантации.
	 */
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Private")
	TArray<FPlantCell> PlantCells;
};
