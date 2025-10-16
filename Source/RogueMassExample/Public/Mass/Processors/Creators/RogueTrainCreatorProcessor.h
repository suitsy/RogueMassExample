// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntitySubsystem.h"
#include "RogueTrainCreatorProcessor.generated.h"

/**
 * 
 */
UCLASS()
class ROGUEMASSEXAMPLE_API URogueTrainCreatorProcessor : public UMassProcessor
{
	GENERATED_BODY()
	
public:
	URogueTrainCreatorProcessor();
	
protected:
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	UPROPERTY(Transient)
	bool bCreated = false;
};