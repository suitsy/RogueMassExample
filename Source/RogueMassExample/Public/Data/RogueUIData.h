// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RogueUIData.generated.h"



USTRUCT(BlueprintType)
struct FRogueSimStats
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly)
	int32 NumPassengers = 0;
	
	UPROPERTY(BlueprintReadOnly)
	int32 NumTrains = 0;
	
	UPROPERTY(BlueprintReadOnly)
	int32 NumCarriages = 0;
	
	UPROPERTY(BlueprintReadOnly)
	int32 NumStations = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 NumPooledEntities = 0;
};