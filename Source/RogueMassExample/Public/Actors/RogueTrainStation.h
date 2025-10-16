// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RogueTrainStation.generated.h"

UCLASS()
class ROGUEMASSEXAMPLE_API ARogueTrainStation : public AActor
{
	GENERATED_BODY()
	
public:
	ARogueTrainStation();

	UPROPERTY(EditAnywhere, Category="Station")
	TArray<FVector> WaitingPoints;

	UPROPERTY(EditAnywhere, Category="Station")
	TArray<FVector> SpawnPoints;

	UFUNCTION(BlueprintCallable, Category="Station")
	float GetStationT() const { return StationT; }

#if WITH_EDITOR
	virtual void OnConstruction(const FTransform& Transform) override;
#endif

private:
	UPROPERTY(EditAnywhere, Category="Station")
	float StationT = 0.f; // computed against world track spline, but editable for manual authoring
	void ComputeStationT();
};
