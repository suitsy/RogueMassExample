// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "RogueDebugDataProcessor.generated.h"

/**
 * 
 */
UCLASS()
class ROGUEMASSEXAMPLE_API URogueDebugDataProcessor : public UMassProcessor
{
	GENERATED_BODY()
	
public:
	URogueDebugDataProcessor();
	
protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery PassengerEntityQuery;
	FMassEntityQuery TrainEntityQuery;
	FMassEntityQuery CarriageEntityQuery;
	FMassEntityQuery StationEntityQuery;
};
