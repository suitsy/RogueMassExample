// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/RogueHud.h"
#include "Data/RogueUIData.h"
#include "Subsystems/RogueTrainWorldSubsystem.h"
#include "UI/RogueSimStatsWidget.h"


ARogueHud::ARogueHud()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ARogueHud::BeginPlay()
{
	Super::BeginPlay();

	if (StatsWidgetClass)
	{
		StatsWidget = CreateWidget<URogueSimStatsWidget>(GetWorld(), StatsWidgetClass);
		if (StatsWidget)
		{
			StatsWidget->AddToViewport();
		}
	}

	// First push immediately, then on a timer
	RefreshStats();
	if (RefreshInterval > 0.f)
	{
		GetWorldTimerManager().SetTimer(RefreshTimer, this, &ARogueHud::RefreshStats, RefreshInterval, true);
	}
}

void ARogueHud::RefreshStats() const
{
	if (!GetWorld() || !StatsWidget) return;

	FRogueSimStats Out;

	// Pull what we can from the train subsystem (cheap)
	if (const URogueTrainWorldSubsystem* TrainSubsystem = GetWorld()->GetSubsystem<URogueTrainWorldSubsystem>())
	{
		Out.NumStations = TrainSubsystem->GetLiveCount(ERogueEntityType::Station);
		Out.NumTrains = TrainSubsystem->GetLiveCount(ERogueEntityType::TrainEngine);
		Out.NumCarriages = TrainSubsystem->GetLiveCount(ERogueEntityType::TrainCarriage);
		Out.NumPassengers = TrainSubsystem->GetLiveCount(ERogueEntityType::Passenger);
		Out.NumPooledEntities = TrainSubsystem->GetPoolCount(ERogueEntityType::Passenger);
	}

	StatsWidget->UpdateStats(Out);
}
