
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameTimeComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameTimeUpdated);

/**
 * @brief Компонент для управления игровым временем.
 *
 * Этот компонент позволяет включать/выключать игровой таймер,
 * управлять скоростью течения времени (через GameTimeRate) и
 * получать уведомления об обновлениях игрового времени.
 * Подходит для симуляций, где требуется контролировать скорость
 * игровых процессов.
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TESTTAST_API UGameTimeComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UGameTimeComponent();

public:
	/**
	 * @brief Включает или выключает игровой таймер.
	 *
	 * @param bEnabled True для включения таймера, False для выключения.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility")
	void ToggleGameTimerEnabled(bool bEnabled);

public:
	/**
	 * @brief Проверяет, активен ли игровой таймер в данный момент.
	 *
	 * @return True, если игровой таймер активен, иначе false.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility")
	FORCEINLINE bool IsGameTimerEnabled() const
	{
		return GetWorld() ? GetWorld()->GetTimerManager().IsTimerActive(GameTimer) : false;
	}

public:
	/**
	 * @brief Коэффициент скорости игрового времени.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.001", ClampMax = "1"), Category = "Game Time")
	float GameTimeRate = 0.05f;

	/**
	 * @brief Флаг, определяющий, должен ли игровой таймер быть активен по умолчанию при старте.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Time")
	bool bDefaultGameTimeEnabled = true;

public:
	/**
	 * @brief Делегат, вызываемый каждый раз, когда игровой таймер обновляется.
	 */
	UPROPERTY(BlueprintAssignable)
	FOnGameTimeUpdated OnGameTimeUpdated;

protected:
	virtual void BeginPlay() override;

private:
	/**
	 * @brief Функция, вызываемая игровым таймером для обновления игрового времени.
	 *
	 * Эта функция выполняет основную логику обновления игрового времени,
	 * например, прогресс симуляции, изменение времени суток и т.д.
	 */
	UFUNCTION()
	void UpdateGameTime();

private:
	/**
	 * @brief Дескриптор таймера, используемый для управления игровым таймером.
	 */
	FTimerHandle GameTimer;
};
