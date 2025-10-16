// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RogueTrainTrack.generated.h"

class USplineComponent;

UCLASS()
class ROGUEMASSEXAMPLE_API ARogueTrainTrack : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ARogueTrainTrack();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Components")
	USplineComponent* SplineComponent = nullptr;
};
