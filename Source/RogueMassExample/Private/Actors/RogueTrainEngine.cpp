// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/RogueTrainEngine.h"


ARogueTrainEngine::ARogueTrainEngine()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
}


