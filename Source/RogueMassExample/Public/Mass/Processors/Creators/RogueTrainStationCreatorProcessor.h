// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "RogueTrainStationCreatorProcessor.generated.h"

/**
 * 
 */
UCLASS()
class ROGUEMASSEXAMPLE_API URogueTrainStationCreatorProcessor : public UMassProcessor
{
	GENERATED_BODY()
	
public:
	URogueTrainStationCreatorProcessor();
	
protected:
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:	
	UPROPERTY(Transient)
	bool bCreated = false;
};
