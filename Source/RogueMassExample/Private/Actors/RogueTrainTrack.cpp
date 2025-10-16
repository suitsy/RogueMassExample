// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/RogueTrainTrack.h"
#include "Components/SplineComponent.h"


// Sets default values
ARogueTrainTrack::ARogueTrainTrack()
{
	PrimaryActorTick.bCanEverTick = false;

	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
}

