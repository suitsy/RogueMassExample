// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/RogueSimStatsWidget.h"
#include "Components/TextBlock.h"


URogueSimStatsWidget::URogueSimStatsWidget(const FObjectInitializer& ObjectInitializer): Super(ObjectInitializer)
{
	SetIsFocusable(false);
}

void URogueSimStatsWidget::UpdateStats(const FRogueSimStats& Stats) const
{
	if (NumStations != nullptr)
	{
		NumStations->SetText(FText::AsNumber(Stats.NumStations)); 
	}
	
	if (NumTrains != nullptr)
	{
		NumTrains->SetText(FText::AsNumber(Stats.NumTrains)); 
	}
	
	if (NumCarriages != nullptr)
	{
		NumCarriages->SetText(FText::AsNumber(Stats.NumCarriages)); 
	}
	
	if (NumPassengers != nullptr)
	{
		NumPassengers->SetText(FText::AsNumber(Stats.NumPassengers)); 
	}
	
	if (NumPooledPassengers != nullptr)
	{
		NumPooledPassengers->SetText(FText::AsNumber(Stats.NumPooledEntities)); 
	}	
}
