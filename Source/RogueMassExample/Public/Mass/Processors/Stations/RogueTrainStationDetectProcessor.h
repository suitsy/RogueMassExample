// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "RogueTrainStationDetectProcessor.generated.h"

/**
 * 
 */
UCLASS()
class ROGUEMASSEXAMPLE_API URogueTrainStationDetectProcessor : public UMassProcessor
{
	GENERATED_BODY()
	
public:
	URogueTrainStationDetectProcessor();
	
protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};
