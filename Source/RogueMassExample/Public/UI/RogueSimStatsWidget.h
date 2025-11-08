// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/RogueUIData.h"
#include "RogueSimStatsWidget.generated.h"

class UTextBlock;
/**
 * 
 */
UCLASS(Abstract, Blueprintable)
class ROGUEMASSEXAMPLE_API URogueSimStatsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	explicit URogueSimStatsWidget(const FObjectInitializer& ObjectInitializer);
	void UpdateStats(const FRogueSimStats& Stats) const;

	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* NumStations;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* NumTrains;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* NumCarriages;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* NumPassengers;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* NumPooledPassengers;
};
