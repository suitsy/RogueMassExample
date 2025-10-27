// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Processors/Debug/RogueDebugDataProcessor.h"
#include "MassCommonFragments.h"
#include "MassExecutionContext.h"
#include "MassNavigationFragments.h"
#include "Mass/Fragments/RogueFragments.h"
#include "Subsystems/RogueTrainWorldSubsystem.h"

URogueDebugDataProcessor::URogueDebugDataProcessor():
	PassengerEntityQuery(*this), TrainEntityQuery(*this), CarriageEntityQuery(*this), StationEntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::AllNetModes);
	ProcessingPhase = EMassProcessingPhase::FrameEnd;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Tasks;
}

void URogueDebugDataProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	// Passengers
	PassengerEntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::All);
	PassengerEntityQuery.AddRequirement<FMassMoveTargetFragment>(EMassFragmentAccess::ReadOnly);	
	PassengerEntityQuery.AddRequirement<FRoguePassengerFragment>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::All);	
	PassengerEntityQuery.AddTagRequirement<FRogueTrainPassengerTag>(EMassFragmentPresence::All);
	PassengerEntityQuery.AddRequirement<FRogueDebugSlotFragment>(EMassFragmentAccess::ReadOnly);
	PassengerEntityQuery.RegisterWithProcessor(*this);	

	// Trains
	TrainEntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::All);
	TrainEntityQuery.AddRequirement<FRogueTrainTrackFollowFragment>(EMassFragmentAccess::ReadOnly);
	TrainEntityQuery.AddRequirement<FRogueTrainStateFragment>(EMassFragmentAccess::ReadOnly);
	TrainEntityQuery.AddTagRequirement<FRogueTrainEngineTag>(EMassFragmentPresence::All);
	TrainEntityQuery.AddRequirement<FRogueDebugSlotFragment>(EMassFragmentAccess::ReadOnly);
	TrainEntityQuery.RegisterWithProcessor(*this);

	// Carriages
	CarriageEntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::All);
	CarriageEntityQuery.AddRequirement<FRogueTrainTrackFollowFragment>(EMassFragmentAccess::ReadOnly);
	CarriageEntityQuery.AddRequirement<FRogueTrainLinkFragment>(EMassFragmentAccess::ReadOnly);
	CarriageEntityQuery.AddRequirement<FRogueCarriageFragment>(EMassFragmentAccess::ReadOnly);
	CarriageEntityQuery.AddTagRequirement<FRogueTrainCarriageTag>(EMassFragmentPresence::All);
	CarriageEntityQuery.AddRequirement<FRogueDebugSlotFragment>(EMassFragmentAccess::ReadOnly);
	CarriageEntityQuery.RegisterWithProcessor(*this);

	// Stations
	StationEntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::All);
	StationEntityQuery.AddRequirement<FRogueStationQueueFragment>(EMassFragmentAccess::ReadOnly);
	StationEntityQuery.AddRequirement<FRogueStationFragment>(EMassFragmentAccess::ReadOnly);
	StationEntityQuery.AddTagRequirement<FRogueTrainStationTag>(EMassFragmentPresence::All);
	StationEntityQuery.AddRequirement<FRogueDebugSlotFragment>(EMassFragmentAccess::ReadOnly);
	StationEntityQuery.RegisterWithProcessor(*this);
}

void URogueDebugDataProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	auto* TrainSubsystem = Context.GetWorld()->GetSubsystem<URogueTrainWorldSubsystem>();
	TWeakObjectPtr<URogueTrainWorldSubsystem> TrainSubsystemWeak = TrainSubsystem;
	if (!TrainSubsystem) return;

	TArray<FRogueDebugPassenger> LocalPassengerSnap;
	LocalPassengerSnap.SetNumZeroed(TrainSubsystem->GetPassengerDebugCapacity());

	TArray<FRogueDebugTrain> LocalTrainSnap;
	LocalTrainSnap.SetNumZeroed(TrainSubsystem->GetTrainDebugCapacity());

	TArray<FRogueDebugCarriage> LocalCarriageSnap;
	LocalCarriageSnap.SetNumZeroed(TrainSubsystem->GetCarriageDebugCapacity());

	TArray<FRogueDebugStation> LocalStationSnap;
	LocalStationSnap.SetNumZeroed(TrainSubsystem->GetStationDebugCapacity());

	// Passengers
	PassengerEntityQuery.ForEachEntityChunk(Context, [&](const FMassExecutionContext& SubContext)
	{
		const TConstArrayView<FTransformFragment> PassengerTransformFragments = SubContext.GetFragmentView<FTransformFragment>();
		const TConstArrayView<FMassMoveTargetFragment> MoveTargetFragments = SubContext.GetFragmentView<FMassMoveTargetFragment>();
		const TConstArrayView<FRoguePassengerFragment> PassengerFragments = SubContext.GetFragmentView<FRoguePassengerFragment>();
		const TConstArrayView<FRogueDebugSlotFragment> PassengerDebugSlots = SubContext.GetFragmentView<FRogueDebugSlotFragment>();
		const int32 NumPassengerEntities = SubContext.GetNumEntities();

		for (int32 PIndex = 0; PIndex < NumPassengerEntities; PIndex++)
		{
			const int32 DebugSlot = PassengerDebugSlots[PIndex].Slot;
			if (!LocalPassengerSnap.IsValidIndex(DebugSlot)) continue;
			
			const FTransform& PTransform = PassengerTransformFragments[PIndex].GetTransform();
			const FMassMoveTargetFragment& MoveTarget = MoveTargetFragments[PIndex];
			const FRoguePassengerFragment& PassengerFragment = PassengerFragments[PIndex];
			const FMassEntityHandle Entity = SubContext.GetEntity(PIndex);

			FRogueDebugPassenger DebugData;
			DebugData.Entity = Entity;
			DebugData.WorldPos = PTransform.GetLocation();
			DebugData.OriginStation = PassengerFragment.OriginStation;
			DebugData.DestStation = PassengerFragment.DestinationStation;
			DebugData.Phase = PassengerFragment.Phase;
			DebugData.bWaiting = PassengerFragment.bWaiting;
			DebugData.WaitingPointIdx = PassengerFragment.WaitingPointIdx;
			DebugData.WaitingSlotIdx = PassengerFragment.WaitingSlotIdx;
			DebugData.Move.DesiredSpeed = MoveTarget.DesiredSpeed;
			DebugData.Move.DistToGoal = MoveTarget.DistanceToGoal;
			DebugData.Move.Target = MoveTarget.Center;
			DebugData.Move.AcceptanceRadius = PassengerFragment.AcceptanceRadius;

			// Write to slot index
			LocalPassengerSnap[DebugSlot] = DebugData;
		}
	});
	
	
	AsyncTask(ENamedThreads::GameThread, [TrainSubsystemWeak, Snap=MoveTemp(LocalPassengerSnap)]() mutable
	{
		if (URogueTrainWorldSubsystem* TSubsystem = TrainSubsystemWeak.Get())
		{
			TSubsystem->SetPassengerDebugSnapshot(MoveTemp(Snap));
		}
	});

	// Trains
	TrainEntityQuery.ForEachEntityChunk(Context, [&](const FMassExecutionContext& SubContext)
	{
		const TConstArrayView<FTransformFragment> TrainTransformFragments = SubContext.GetFragmentView<FTransformFragment>();
		const TConstArrayView<FRogueTrainTrackFollowFragment> TrainFollowViews = SubContext.GetFragmentView<FRogueTrainTrackFollowFragment>();
		const TConstArrayView<FRogueTrainStateFragment> StateViews = SubContext.GetFragmentView<FRogueTrainStateFragment>();
		const TConstArrayView<FRogueDebugSlotFragment> TrainDebugSlots = SubContext.GetFragmentView<FRogueDebugSlotFragment>();
		const int32 NumTrainEntities = SubContext.GetNumEntities();

		for (int32 TIndex = 0; TIndex < NumTrainEntities; TIndex++)
		{
			const int32 DebugSlot = TrainDebugSlots[TIndex].Slot;
			if (!LocalTrainSnap.IsValidIndex(DebugSlot)) continue;

			const FTransform& TTransform = TrainTransformFragments[TIndex].GetTransform();
			const FRogueTrainTrackFollowFragment& Follow = TrainFollowViews[TIndex];
			const FRogueTrainStateFragment& State = StateViews[TIndex];
			const FMassEntityHandle Entity = SubContext.GetEntity(TIndex);

			FRogueDebugTrain DebugData;
			DebugData.Entity = Entity;
			DebugData.Alpha = Follow.Alpha; 
			DebugData.Speed = Follow.Speed;
			DebugData.WorldPos = TTransform.GetLocation();
			DebugData.bIsStopping = State.bIsStopping;
			DebugData.bAtStation = State.bAtStation;
			DebugData.TargetStationIdx = State.TargetStationIdx;
			DebugData.StationTimeRemaining = State.StationTimeRemaining;
			DebugData.TrainPhase = State.StationTrainPhase;
			DebugData.HeadwaySpeedScale = State.HeadwaySpeedScale;

			// Write to slot index
			LocalTrainSnap[DebugSlot] = DebugData;
		}
	});	
	
	AsyncTask(ENamedThreads::GameThread, [TrainSubsystemWeak, Snap=MoveTemp(LocalTrainSnap)]() mutable
	{
		if (URogueTrainWorldSubsystem* TSubsystem = TrainSubsystemWeak.Get())
		{
			TSubsystem->SetTrainDebugSnapshot(MoveTemp(Snap));
		}
	});

	// Carriages
	CarriageEntityQuery.ForEachEntityChunk(Context, [&](const FMassExecutionContext& SubContext)
	{
		const TConstArrayView<FTransformFragment> CarriageTransformFragments = SubContext.GetFragmentView<FTransformFragment>();
		const TConstArrayView<FRogueTrainLinkFragment> CarriageLinkFragments = SubContext.GetFragmentView<FRogueTrainLinkFragment>();
		const TConstArrayView<FRogueTrainTrackFollowFragment> CarriageFollowViews = SubContext.GetFragmentView<FRogueTrainTrackFollowFragment>();
		const TConstArrayView<FRogueCarriageFragment> CarriageFragments = SubContext.GetFragmentView<FRogueCarriageFragment>();
		const TConstArrayView<FRogueDebugSlotFragment> CarriageSlots = SubContext.GetFragmentView<FRogueDebugSlotFragment>();
		const int32 NumCarriageEntities = SubContext.GetNumEntities();
		
		for (int32 CIndex = 0; CIndex < NumCarriageEntities; CIndex++)
		{
			const int32 DebugSlot = CarriageSlots[CIndex].Slot;
			if (!LocalPassengerSnap.IsValidIndex(DebugSlot)) continue;
			
			const FTransform& CTransform = CarriageTransformFragments[CIndex].GetTransform();
			const FRogueTrainLinkFragment& LinkFragment = CarriageLinkFragments[CIndex];
			const FRogueTrainTrackFollowFragment& FollowFragment = CarriageFollowViews[CIndex];
			const FRogueCarriageFragment& CarriageFragment = CarriageFragments[CIndex];
			const FMassEntityHandle Entity = SubContext.GetEntity(CIndex);

			FRogueDebugCarriage DebugData;
			DebugData.Entity = Entity;
			DebugData.Alpha = FollowFragment.Alpha; 
			DebugData.Speed = FollowFragment.Speed;
			DebugData.WorldPos = CTransform.GetLocation();
			DebugData.IndexInTrain = LinkFragment.CarriageIndex;
			DebugData.Spacing = LinkFragment.SpacingMeters;
			DebugData.Capacity = CarriageFragment.Capacity;
			DebugData.Occupants = CarriageFragment.Occupants.Num();

			// Write to slot index
			LocalCarriageSnap[DebugSlot] = DebugData;
		}
	});
	
	AsyncTask(ENamedThreads::GameThread, [TrainSubsystemWeak, Snap=MoveTemp(LocalCarriageSnap)]() mutable
	{
		if (URogueTrainWorldSubsystem* TSubsystem = TrainSubsystemWeak.Get())
		{
			TSubsystem->SetCarriageDebugSnapshot(MoveTemp(Snap));
		}
	});

	// Stations
	StationEntityQuery.ForEachEntityChunk(Context, [&](const FMassExecutionContext& SubContext)
	{
		const TConstArrayView<FTransformFragment> StationTransformFragments = SubContext.GetFragmentView<FTransformFragment>();
		const TConstArrayView<FRogueStationQueueFragment> StationQueueFragments = SubContext.GetFragmentView<FRogueStationQueueFragment>();
		const TConstArrayView<FRogueStationFragment> StationFragments = SubContext.GetFragmentView<FRogueStationFragment>();
		const TConstArrayView<FRogueDebugSlotFragment> StationDebugSlots = SubContext.GetFragmentView<FRogueDebugSlotFragment>();
		const int32 NumStationEntities = SubContext.GetNumEntities();
		
		for (int32 SIndex = 0; SIndex < NumStationEntities; SIndex++)
		{
			const int32 DebugSlot = StationDebugSlots[SIndex].Slot;
			if (!LocalStationSnap.IsValidIndex(DebugSlot)) continue;
			
			const FTransform& STransform = StationTransformFragments[SIndex].GetTransform();
			const FRogueStationQueueFragment& QueueFragment = StationQueueFragments[SIndex];
			const FRogueStationFragment& StationFragment = StationFragments[SIndex];
			const FMassEntityHandle Entity = SubContext.GetEntity(SIndex);

			FRogueDebugStation DebugData;
			DebugData.Entity = Entity;
			DebugData.StationIdx = StationFragment.StationIndex;
			DebugData.TrackAlpha = StationFragment.StationAlpha;
			DebugData.WorldPos = STransform.GetLocation();
			DebugData.Grids.Init(FRogueDebugWaitingGrid(), QueueFragment.Grids.Num());

			// Queues
			int32 TotalWaitingCount = 0;
			int32 i = 0;
			for (const TPair<int32, FRogueWaitingGrid>& Pair : QueueFragment.Grids)
			{
				const FRogueWaitingGrid& WaitingGrid = Pair.Value;
				FRogueDebugWaitingGrid& GridData = DebugData.Grids[i++];
				GridData.WaitingPointIdx = Pair.Key;
				GridData.Slots = WaitingGrid.SlotPositions.Num();

				int32 WaitingLocalGridCount = 0;
				for (int j = 0; j < WaitingGrid.OccupiedBy.Num(); ++j)
				{
					if (WaitingGrid.OccupiedBy[j].IsValid())
					{
						WaitingLocalGridCount++;
					}
				}

				GridData.Free = GridData.Slots - WaitingLocalGridCount;
				GridData.Occupied = WaitingLocalGridCount;
				TotalWaitingCount += WaitingLocalGridCount;
			}
			
			DebugData.TotalWaiting = TotalWaitingCount;
			DebugData.TotalSpawnPoints = QueueFragment.SpawnPoints.Num();
			DebugData.TotalWaitingPoints = QueueFragment.WaitingPoints.Num();

			// Write to slot index
			LocalStationSnap[DebugSlot] = DebugData;
		}
	});
	
	AsyncTask(ENamedThreads::GameThread, [TrainSubsystemWeak, Snap=MoveTemp(LocalStationSnap)]() mutable
	{
		if (URogueTrainWorldSubsystem* TSubsystem = TrainSubsystemWeak.Get())
		{
			TSubsystem->SetStationDebugSnapshot(MoveTemp(Snap));
		}
	});
}
