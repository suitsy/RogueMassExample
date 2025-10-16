// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RogueTrainCarriage.generated.h"

class ARogueTrainEngine;

UCLASS()
class ROGUEMASSEXAMPLE_API ARogueTrainCarriage : public AActor
{
	GENERATED_BODY()
	
public:
	ARogueTrainCarriage();

	UPROPERTY(EditAnywhere, Category="Train")
	TWeakObjectPtr<ARogueTrainEngine> Engine;

	UPROPERTY(EditAnywhere, Category="Train", meta=(ClampMin="1"))
	int32 CarriageIndex = 1;

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
	UStaticMeshComponent* MeshComponent;
};
