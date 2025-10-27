// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Processors/Passengers/RoguePassengerMovementProcessor.h"
#include "MassCommonFragments.h"
#include "MassCommonTypes.h"
#include "MassExecutionContext.h"
#include "Subsystems/RogueTrainWorldSubsystem.h"
#include "Utilities/RoguePassengerUtility.h"
#include "Utilities/RogueStationQueueUtility.h"

URoguePassengerMovementProcessor::URoguePassengerMovementProcessor(): EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::AllNetModes);
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Movement;
}

void URoguePassengerMovementProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{	
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite, EMassFragmentPresence::All);
	EntityQuery.AddRequirement<FMassMoveTargetFragment>(EMassFragmentAccess::ReadWrite);	
	EntityQuery.AddConstSharedRequirement<FMassMovementParameters>(EMassFragmentPresence::All);
	EntityQuery.AddRequirement<FRoguePassengerFragment>(EMassFragmentAccess::ReadWrite, EMassFragmentPresence::All);	
	EntityQuery.AddTagRequirement<FRogueTrainPassengerTag>(EMassFragmentPresence::All);
	EntityQuery.RegisterWithProcessor(*this);	
}

void URoguePassengerMovementProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	auto* TrainSubsystem = Context.GetWorld()->GetSubsystem<URogueTrainWorldSubsystem>();
	if (!TrainSubsystem) return;
	
	const FRogueTrackSharedFragment& TrackSharedFragment = TrainSubsystem->GetTrackShared();
	if (!TrackSharedFragment.IsValid()) return;

	const float Time = Context.GetWorld()->GetTimeSeconds();

	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& SubContext)
	{
		const TConstArrayView<FTransformFragment> TransformFragments = SubContext.GetMutableFragmentView<FTransformFragment>();
		const TArrayView<FMassMoveTargetFragment> NavTargetList = SubContext.GetMutableFragmentView<FMassMoveTargetFragment>();
		const FMassMovementParameters& MoveParams = SubContext.GetConstSharedFragment<FMassMovementParameters>();
		const TArrayView<FRoguePassengerFragment> PassengerFragments = SubContext.GetMutableFragmentView<FRoguePassengerFragment>();
		const int32 NumEntities = SubContext.GetNumEntities();
		
		for (int32 EntityIndex = 0; EntityIndex < NumEntities; EntityIndex++)
		{
			const FTransform& PTransform = TransformFragments[EntityIndex].GetTransform();
			FMassMoveTargetFragment& MoveTarget = NavTargetList[EntityIndex];
			const FMassEntityHandle PassengerHandle = SubContext.GetEntity(EntityIndex);
			FRoguePassengerFragment& PassengerFragment = PassengerFragments[EntityIndex];
			const FMassEntityHandle Entity = SubContext.GetEntity(EntityIndex);

			// Check we have a valid station and no target has been assigned yet
			if (PassengerFragment.OriginStation.IsValid() && PassengerFragment.WaitingPointIdx == INDEX_NONE)
			{				
				// Assign a waiting point 
				AssignWaitingPoint(EntityManager, PassengerFragment, Entity);

				if (PassengerFragment.WaitingSlotIdx == INDEX_NONE)
				{
					MoveTarget.IntentAtGoal = EMassMovementAction::Stand;
					MoveTarget.DesiredSpeed = FMassInt16Real(0.f);
					continue; // No slot free, stay at spawn
				}
			}

			// If we have a move target, update movement towards it			
			if (PassengerFragment.Phase != ERoguePassengerPhase::RideOnTrain && !PassengerFragment.bWaiting)
			{
				MoveToTarget(PassengerFragment, MoveTarget, MoveParams, PTransform, PassengerFragment.Target);
			}			

			// Handle phase-specific logic, destination arrival, boarding, departing and waiting
			switch (PassengerFragment.Phase)
			{
				case ERoguePassengerPhase::ToStationWaitingPoint: ToStationWaitingPoint(EntityManager, PassengerFragment, PTransform, PassengerHandle, Time); break;
				case ERoguePassengerPhase::ToAssignedCarriage: ToAssignedCarriage(EntityManager, SubContext, PassengerFragment, PTransform, PassengerHandle); break;
				case ERoguePassengerPhase::RideOnTrain: break; // Riding, do nothing
				case ERoguePassengerPhase::UnloadAtStation: UnloadAtStation(EntityManager, PassengerFragment, PTransform); break;
				case ERoguePassengerPhase::ToPostUnloadWaitingPoint: ToPostUnloadWaitingPoint(EntityManager, PassengerFragment, PTransform); break;
				case ERoguePassengerPhase::ToExitSpawn: ToExitSpawn(EntityManager, TrainSubsystem, SubContext, PassengerFragment, PTransform, PassengerHandle); break;
				default:
					break;
			}
		}
	});
}

void URoguePassengerMovementProcessor::AssignWaitingPoint(const FMassEntityManager& EntityManager, FRoguePassengerFragment& PassengerFragment, const FMassEntityHandle& Entity)
{
	if (auto* StationQueueFragment = EntityManager.GetFragmentDataPtr<FRogueStationQueueFragment>(PassengerFragment.OriginStation))
	{
		// Choose a random waiting point at that station
		PassengerFragment.WaitingPointIdx = (StationQueueFragment->WaitingPoints.Num() > 0)
			? FMath::RandRange(0, StationQueueFragment->WaitingPoints.Num() - 1)
			: INDEX_NONE;
		if (PassengerFragment.WaitingPointIdx == INDEX_NONE) return;

		// Assign a waiting slot at that waiting point
		FVector SlotPosition;
		const int32 SlotIdx = RogueStationQueueUtility::ClaimWaitingSlot(StationQueueFragment, PassengerFragment.WaitingPointIdx, Entity, SlotPosition);
		PassengerFragment.WaitingSlotIdx = SlotIdx;
		if (SlotIdx == INDEX_NONE) return;

		// Assign move target to waiting point
		PassengerFragment.Target = SlotPosition;
	}
}

void URoguePassengerMovementProcessor::MoveToTarget(const FRoguePassengerFragment& PassengerFragment, FMassMoveTargetFragment& MoveTarget, const FMassMovementParameters& MoveParams,
	const FTransform& PTransform, const FVector& TargetDestination)
{
	FVector Delta  = PassengerFragment.Target - PTransform.GetLocation();
	Delta.Z = 0.f;
	const float DistToGoal = Delta.Size();
	
	if (DistToGoal > PassengerFragment.AcceptanceRadius)
	{
		MoveTarget.Center = TargetDestination;
		MoveTarget.DistanceToGoal = DistToGoal;
		MoveTarget.Forward = Delta.GetSafeNormal();
		
		constexpr float SlowdownRadius = 120.f; 
		const float t = FMath::Clamp(DistToGoal / SlowdownRadius, 0.f, 1.f);
		const float TargetSpeed = t * MoveParams.DefaultDesiredSpeed; 
		MoveTarget.DesiredSpeed = FMassInt16Real(TargetSpeed);
	}
}

void URoguePassengerMovementProcessor::ToStationWaitingPoint(const FMassEntityManager& EntityManager, FRoguePassengerFragment& PassengerFragment,
	const FTransform& PTransform, const FMassEntityHandle PassengerHandle, const float Time)
{
	// Arrived? enqueue into that waiting-point queue if not already queued, idle until boarding - boarding handled by station ops processor
	if (FVector::DistSquared(PTransform.GetLocation(), PassengerFragment.Target) <= FMath::Square(PassengerFragment.AcceptanceRadius) && !PassengerFragment.bWaiting)
	{
		if (!PassengerFragment.OriginStation.IsValid()) return;
		
		if (auto* StationQueueFragment = EntityManager.GetFragmentDataPtr<FRogueStationQueueFragment>(PassengerFragment.OriginStation))
		{
			RoguePassengerQueueUtility::EnqueueAtWaitingPoint(*StationQueueFragment, PassengerFragment.WaitingPointIdx, PassengerHandle, PassengerFragment.DestinationStation, Time, /*prio*/0);
			PassengerFragment.bWaiting = true;
			PassengerFragment.Target = PTransform.GetLocation();
		}		
	}
}

void URoguePassengerMovementProcessor::ToAssignedCarriage(const FMassEntityManager& EntityManager, const FMassExecutionContext& Context, FRoguePassengerFragment& PassengerFragment,
	 const FTransform& PTransform, const FMassEntityHandle PassengerHandle)
{
	// If we were boarded already, VehicleHandle is set, head to the carriage door (carriage transform)
	if (PassengerFragment.VehicleHandle.IsSet() && EntityManager.IsEntityValid(PassengerFragment.VehicleHandle))
	{
		if (const auto* CarriageTransformFragment = EntityManager.GetFragmentDataPtr<FTransformFragment>(PassengerFragment.VehicleHandle))
		{
			PassengerFragment.Target = CarriageTransformFragment->GetTransform().GetLocation();

			if (FVector::DistSquared(PTransform.GetLocation(), PassengerFragment.Target) <= FMath::Square(PassengerFragment.AcceptanceRadius))
			{				
				RoguePassengerUtility::HidePassenger(EntityManager, PassengerHandle);
				PassengerFragment.Phase = ERoguePassengerPhase::RideOnTrain;
				PassengerFragment.WaitingPointIdx = INDEX_NONE;
				PassengerFragment.WaitingSlotIdx = INDEX_NONE;
			}
		}
	}
}

void URoguePassengerMovementProcessor::UnloadAtStation(const FMassEntityManager& EntityManager, FRoguePassengerFragment& PassengerFragment, const FTransform& PTransform)
{
	if (!PassengerFragment.DestinationStation.IsValid()) return;

	if (const auto* StationQueueFragment = EntityManager.GetFragmentDataPtr<FRogueStationQueueFragment>(PassengerFragment.DestinationStation))
	{
		const int32 WaitingPoint = RoguePassengerUtility::FindNearestIndex(StationQueueFragment->WaitingPoints, PTransform.GetLocation());				
		if (StationQueueFragment->WaitingPoints.IsValidIndex(WaitingPoint))
		{
			PassengerFragment.WaitingPointIdx = WaitingPoint;
			PassengerFragment.Target = StationQueueFragment->WaitingPoints[WaitingPoint];
			PassengerFragment.Phase = ERoguePassengerPhase::ToPostUnloadWaitingPoint;
		}
	}
}

void URoguePassengerMovementProcessor::ToPostUnloadWaitingPoint(const FMassEntityManager& EntityManager, FRoguePassengerFragment& PassengerFragment, const FTransform& PTransform)
{
	if (FVector::DistSquared(PTransform.GetLocation(), PassengerFragment.Target) <= FMath::Square(PassengerFragment.AcceptanceRadius * 2.f))
	{
		// Immediately head to nearest exit spawn to leave the world
		if (const auto* StationQueueFragment = EntityManager.GetFragmentDataPtr<FRogueStationQueueFragment>(PassengerFragment.DestinationStation))
		{
			const int32 ExitIdx = RoguePassengerUtility::FindNearestIndex(StationQueueFragment->SpawnPoints, PTransform.GetLocation());			
			if (StationQueueFragment->SpawnPoints.IsValidIndex(ExitIdx))
			{
				PassengerFragment.Target = StationQueueFragment->SpawnPoints[ExitIdx];
			}
		}
		
		PassengerFragment.Phase = ERoguePassengerPhase::ToExitSpawn;
	}
}

void URoguePassengerMovementProcessor::ToExitSpawn(const FMassEntityManager& EntityManager, URogueTrainWorldSubsystem* TrainSubsystem, const FMassExecutionContext& Context, FRoguePassengerFragment& PassengerFragment,
	 const FTransform& PTransform, const FMassEntityHandle PassengerHandle)
{
	if (FVector::DistSquared(PTransform.GetLocation(), PassengerFragment.Target) <= FMath::Square(PassengerFragment.AcceptanceRadius))
	{
		if (auto* StationQueueFragment = EntityManager.GetFragmentDataPtr<FRogueStationQueueFragment>(PassengerFragment.OriginStation))
		{
			if (PassengerFragment.bWaiting && PassengerFragment.WaitingPointIdx != INDEX_NONE && PassengerFragment.WaitingSlotIdx != INDEX_NONE)
			{
				RogueStationQueueUtility::ReleaseSlot(*StationQueueFragment, PassengerFragment);
			}
		}
		
		// Return to pool / mark for destruction
		PassengerFragment.Phase = ERoguePassengerPhase::Pool;
		TrainSubsystem->EnqueueEntityToPool(PassengerHandle, Context, ERogueEntityType::Passenger);
	}
}
