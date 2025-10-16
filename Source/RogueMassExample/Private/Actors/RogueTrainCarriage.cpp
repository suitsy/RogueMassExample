// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/RogueTrainCarriage.h"


ARogueTrainCarriage::ARogueTrainCarriage()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
}