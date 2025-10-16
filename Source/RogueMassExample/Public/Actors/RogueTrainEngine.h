// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RogueTrainEngine.generated.h"

UCLASS()
class ROGUEMASSEXAMPLE_API ARogueTrainEngine : public AActor
{
	GENERATED_BODY()
	
public:
	ARogueTrainEngine();

	UPROPERTY(EditAnywhere, Category="Train")
	float StartAlpha = 0.f;

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
	UStaticMeshComponent* MeshComponent;
};
