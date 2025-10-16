// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "Mass/Fragments/RogueFragments.h"
#include "RogueTrainCarriageFollowProcessor.generated.h"

/**
 * 
 */
UCLASS()
class ROGUEMASSEXAMPLE_API URogueTrainCarriageFollowProcessor : public UMassProcessor
{
	GENERATED_BODY()
public:
	URogueTrainCarriageFollowProcessor();
	
protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery EntityQuery;

private:
	static const FRogueSplineFollowFragment* GetLeadFollow(const FMassEntityView& View, const FRogueTrainLinkFragment& Link);
};
