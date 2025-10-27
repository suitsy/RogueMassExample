// Fill out your copyright notice in the Description page of Project Settings.


#include "Utilities/RoguePassengerUtility.h"
#include "MassCommandBuffer.h"
#include "MassCommands.h"
#include "MassCommonFragments.h"
#include "MassMovementFragments.h"
#include "MassNavigationFragments.h"
#include "MassRepresentationFragments.h"
#include "Subsystems/RogueTrainWorldSubsystem.h"


void RoguePassengerQueueUtility::EnqueueAtWaitingPoint(FRogueStationQueueFragment& StationQueueFragment, const int32 WaitingPointIdx,
	const FMassEntityHandle Passenger, const FMassEntityHandle DestStation, const float Time, const int32 Priority)
{
	TArray<FRoguePassengerQueueEntry>* Entries = StationQueueFragment.QueuesByWaitingPoint.Find(WaitingPointIdx);
	if (!Entries) { Entries = &StationQueueFragment.QueuesByWaitingPoint.Add(WaitingPointIdx); }
	
	FRoguePassengerQueueEntry QueueEntry;
	QueueEntry.Passenger = Passenger;
	QueueEntry.DestStation = DestStation;
	QueueEntry.WaitingPointIdx = WaitingPointIdx;
	QueueEntry.EnqueuedGameTime = Time;
	QueueEntry.Priority = Priority;
	
	Entries->Add(MoveTemp(QueueEntry));
}

bool RoguePassengerQueueUtility::DequeueFromWaitingPoint(FRogueStationQueueFragment& StationQueueFragment, const int32 WaitingPointIdx, FRoguePassengerQueueEntry& Out)
{
	TArray<FRoguePassengerQueueEntry>* Entries = StationQueueFragment.QueuesByWaitingPoint.Find(WaitingPointIdx);
	if (!Entries || Entries->Num() == 0) return false;

	int32 BestIdx = INDEX_NONE;
	int32 BestPriority = TNumericLimits<int32>::Min();
	float BestTime = 0.f;

	for (int32 i = 0; i < Entries->Num(); ++i)
	{
		const auto& QueueEntry = (*Entries)[i];

		if (BestIdx == INDEX_NONE || QueueEntry.Priority > BestPriority || (QueueEntry.Priority == BestPriority && QueueEntry.EnqueuedGameTime < BestTime))
		{
			BestIdx = i;
			BestPriority = QueueEntry.Priority;
			BestTime = QueueEntry.EnqueuedGameTime;
		}
	}

	if (BestIdx != INDEX_NONE)
	{
		Out = (*Entries)[BestIdx];
		Entries->RemoveAtSwap(BestIdx, 1, EAllowShrinking::No);
		return true;
	}
	
	return false;
}

void RoguePassengerUtility::Disembark(const FMassEntityManager& EntityManager, const FMassExecutionContext& Context, FRogueCarriageFragment& CarriageFragment, const int32 Index, const FVector& Location)
{
	const FMassEntityHandle Passenger = CarriageFragment.Occupants[Index];
	if (IsHandleValid(EntityManager, Passenger))
	{
		if (FRoguePassengerFragment* PassengerFragment = EntityManager.GetFragmentDataPtr<FRoguePassengerFragment>(Passenger))
		{
			RoguePassengerUtility::ShowPassenger(EntityManager, Passenger, Location);
			PassengerFragment->VehicleHandle = FMassEntityHandle();
			PassengerFragment->WaitingPointIdx = INDEX_NONE; 
			PassengerFragment->Phase = ERoguePassengerPhase::UnloadAtStation;
		}
	}
	
	CarriageFragment.Occupants.RemoveAtSwap(Index, 1, EAllowShrinking::No);
}

bool RoguePassengerUtility::TryBoard(const FMassEntityManager& EntityManager, const FMassExecutionContext& Context, const FMassEntityHandle Passenger, const FMassEntityHandle CarriageEntity, FRogueCarriageFragment& CarriageFragment)
{
	if (CarriageFragment.Occupants.Num() >= CarriageFragment.Capacity) return false;
	if (!IsHandleValid(EntityManager, Passenger)) return false;

	// attach
	if (FRoguePassengerFragment* PassengerFragment = EntityManager.GetFragmentDataPtr<FRoguePassengerFragment>(Passenger))
	{
		PassengerFragment->VehicleHandle = CarriageEntity;
		PassengerFragment->Phase = ERoguePassengerPhase::ToAssignedCarriage;
	}

	CarriageFragment.Occupants.Add(Passenger);
	
	return true;
}

void RoguePassengerUtility::HidePassenger(const FMassEntityManager& EntityManager, const FMassEntityHandle EntityHandle)
{
	// PlatformConfig off + stash underground (simple, consistent with your pool pattern)
	if (auto* TransformFragment = EntityManager.GetFragmentDataPtr<FTransformFragment>(EntityHandle))
	{
		FTransform& Transform = TransformFragment->GetMutableTransform();
		Transform.SetLocation(FVector(0,0,-100000.f));		
		
		if (auto* PassengerFragment = EntityManager.GetFragmentDataPtr<FRoguePassengerFragment>(EntityHandle))
		{
			PassengerFragment->Target = TransformFragment->GetTransform().GetLocation();
		}		

		if (auto* MoveTargetFragment = EntityManager.GetFragmentDataPtr<FMassMoveTargetFragment>(EntityHandle))
		{
			MoveTargetFragment->Center         = Transform.GetLocation();
			MoveTargetFragment->DistanceToGoal = 0.f;
			MoveTargetFragment->DesiredSpeed   = FMassInt16Real(0.f);
			MoveTargetFragment->Forward        = FVector::ZeroVector;
			MoveTargetFragment->IntentAtGoal   = EMassMovementAction::Stand;
		}
	
		if (auto* Vel = EntityManager.GetFragmentDataPtr<FMassVelocityFragment>(EntityHandle))
		{
			Vel->Value = FVector::ZeroVector;
		}
	}
	
	if (auto* LOD = EntityManager.GetFragmentDataPtr<FMassRepresentationLODFragment>(EntityHandle))
	{
		LOD->LOD = EMassLOD::Off;
	}
}

void RoguePassengerUtility::ShowPassenger(const FMassEntityManager& EntityManager, const FMassEntityHandle EntityHandle, const FVector& ShowLocation)
{
	// PlatformConfig off + stash underground (simple, consistent with your pool pattern)
	if (!ShowLocation.IsNearlyZero())
	{
		if (auto* TransformFragment = EntityManager.GetFragmentDataPtr<FTransformFragment>(EntityHandle))
		{				
			FTransform& Transform = TransformFragment->GetMutableTransform();
			Transform.SetLocation(ShowLocation);
		}
	}
	
	if (auto* LOD = EntityManager.GetFragmentDataPtr<FMassRepresentationLODFragment>(EntityHandle))
	{
		LOD->LOD = EMassLOD::Low;
	}
}

int32 RoguePassengerUtility::FindNearestIndex(const TArray<FVector>& Points, const FVector& From)
{
	int32 Best = INDEX_NONE;
	float BestDistSq = TNumericLimits<float>::Max();
	
	for (int32 i = 0; i < Points.Num(); ++i)
	{
		const float DistSq = FVector::DistSquared(Points[i], From);
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = i;
		}
	}
	
	return Best;
}

bool RoguePassengerUtility::SnapToPlatform(const UWorld* WorldContext, FVector& InOutPos, const float MaxStepUp, const float MaxDrop)
{
	const FVector Start = InOutPos + FVector(0,0, MaxStepUp);
	const FVector End = InOutPos - FVector(0,0, MaxDrop);

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SnapToPlatform), /*bTraceComplex*/ false);
	Params.bReturnPhysicalMaterial = false;

	if (WorldContext->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params) && Hit.bBlockingHit)
	{
		InOutPos.Z = Hit.ImpactPoint.Z;
		return true;
	}
	
	return false;
}
