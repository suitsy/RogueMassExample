// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "RoguePassengerSpawnProcessor.generated.h"

/**
 * 
 */
UCLASS()
class ROGUEMASSEXAMPLE_API URoguePassengerSpawnProcessor : public UMassProcessor
{
	GENERATED_BODY()
public:
	URoguePassengerSpawnProcessor();
	
protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery EntityQuery;

private:
	float SpawnAccumulator = 0.f;
};
