// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassMovementFragments.h"
#include "MassNavigationFragments.h"
#include "MassProcessor.h"
#include "Mass/Fragments/RogueFragments.h"
#include "RoguePassengerMovementProcessor.generated.h"

class URogueTrainWorldSubsystem;

/**
 * 
 */
UCLASS()
class ROGUEMASSEXAMPLE_API URoguePassengerMovementProcessor : public UMassProcessor
{
	GENERATED_BODY()
	
public:
	URoguePassengerMovementProcessor();
	
protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery EntityQuery;

private:
	static void AssignWaitingPoint(const FMassEntityManager& EntityManager, FRoguePassengerFragment& PassengerFragment, const FMassEntityHandle& Entity);
	static void MoveToTarget(const FRoguePassengerFragment& PassengerFragment, FMassMoveTargetFragment& MoveTarget, const FMassMovementParameters& MoveParams,const FTransform& PTransform, const FVector& TargetDestination);
	static void ToStationWaitingPoint(const FMassEntityManager& EntityManager, FRoguePassengerFragment& PassengerFragment,
		const FTransform& PTransform, const FMassEntityHandle PassengerHandle, const float Time);
	static void ToAssignedCarriage(const FMassEntityManager& EntityManager, const FMassExecutionContext& Context, FRoguePassengerFragment& PassengerFragment,
		const FTransform& PTransform, const FMassEntityHandle PassengerHandle);
	static void UnloadAtStation(const FMassEntityManager& EntityManager, FRoguePassengerFragment& PassengerFragment, const FTransform& PTransform);
	static void ToPostUnloadWaitingPoint(const FMassEntityManager& EntityManager, FRoguePassengerFragment& PassengerFragment,
		const FTransform& PTransform);
	static void ToExitSpawn(const FMassEntityManager& EntityManager, URogueTrainWorldSubsystem* TrainSubsystem, const FMassExecutionContext& Context, FRoguePassengerFragment& PassengerFragment,
		const FTransform& PTransform, const FMassEntityHandle PassengerHandle);
};
