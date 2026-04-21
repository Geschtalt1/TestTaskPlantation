

#include "Components/GameTimeComponent.h"

UGameTimeComponent::UGameTimeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UGameTimeComponent::BeginPlay()
{
	Super::BeginPlay();
	
	ToggleGameTimerEnabled(bDefaultGameTimeEnabled);
}

void UGameTimeComponent::ToggleGameTimerEnabled(bool bEnabled)
{
	if (IsGameTimerEnabled() == bEnabled)
	{
		return;
	}

	FTimerManager& TimerManager = GetWorld()->GetTimerManager();
	if (bEnabled)
	{
		TimerManager.SetTimer(
			GameTimer,
			this,
			&UGameTimeComponent::UpdateGameTime,
			GameTimeRate,
			true
		);
	}
	else
	{
		TimerManager.ClearTimer(GameTimer);
	}
}

void UGameTimeComponent::UpdateGameTime()
{
	OnGameTimeUpdated.Broadcast();
}