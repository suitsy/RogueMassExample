// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Processors/Stations/RogueTrainStationOpsProcessor.h"
#include "MassCommonFragments.h"
#include "MassCommonTypes.h"
#include "MassExecutionContext.h"
#include "Data/RogueDeveloperSettings.h"
#include "Mass/Processors/Stations/RogueTrainStationDetectProcessor.h"
#include "Subsystems/RogueTrainWorldSubsystem.h"
#include "Utilities/RoguePassengerUtility.h"
#include "Utilities/RogueStationQueueUtility.h"


URogueTrainStationOpsProcessor::URogueTrainStationOpsProcessor(): EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::AllNetModes);
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
    ExecutionOrder.ExecuteAfter.Add(URogueTrainStationDetectProcessor::StaticClass()->GetFName());
}

void URogueTrainStationOpsProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FRogueTrainStateFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddTagRequirement<FRogueTrainEngineTag>(EMassFragmentPresence::All);
	EntityQuery.RegisterWithProcessor(*this);
}

void URogueTrainStationOpsProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	auto* TrainSubsystem = Context.GetWorld()->GetSubsystem<URogueTrainWorldSubsystem>();
	if (!TrainSubsystem) return;

	const FRogueTrackSharedFragment& TrackSharedFragment = TrainSubsystem->GetTrackShared();
	if (!TrackSharedFragment.IsValid()) return;

	const auto* Settings = GetDefault<URogueDeveloperSettings>();
	if (!Settings) return;

	const float DepartureTime = Settings->DepartureTimeSeconds;
	const int32 MaxLoadPerTickPerCar = Settings->MaxLoadPerTickPerCarriage;
	const float StationStateSwitchTime = (Settings->MaxDwellTimeSeconds * 0.5f) + (DepartureTime * 0.5f);
	const float CurrentTime = Context.GetWorld()->GetTimeSeconds();

	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& SubContext)
	{
		const TArrayView<FRogueTrainStateFragment> StateView = SubContext.GetMutableFragmentView<FRogueTrainStateFragment>();

		for (int32 i = 0; i < SubContext.GetNumEntities(); ++i)
		{
			auto& State = StateView[i];
			
			// Skip if not at station
            if (!State.bAtStation || State.TargetStationIdx == INDEX_NONE) continue;

			switch (State.StationTrainPhase)
			{
				case ERogueStationTrainPhase::NotStopped: State.StationTrainPhase = ERogueStationTrainPhase::Arriving;
					break;
				case ERogueStationTrainPhase::Arriving:
				{
					// Unload passengers on first half of dwell time
					if (State.StationTimeRemaining >= StationStateSwitchTime)
					{
						State.StationTrainPhase = ERogueStationTrainPhase::Unloading;
					}
				}
				break;
				case ERogueStationTrainPhase::Unloading:
				{
					// Load passengers on second half of dwell time
				   if (State.StationTimeRemaining < StationStateSwitchTime)
				   {
					   State.StationTrainPhase = ERogueStationTrainPhase::Loading;
				   }
				}
				break;
				case ERogueStationTrainPhase::Loading:
				{
					// Clear flags when dwell time is almost up
					if (State.StationTimeRemaining < DepartureTime)
					{
						State.StationTrainPhase = ERogueStationTrainPhase::Departing;
					}
				}
				break;
				case ERogueStationTrainPhase::Departing:
					break;
			}			

			// Only proceed if loading or unloading
			if (State.StationTrainPhase != ERogueStationTrainPhase::Loading && State.StationTrainPhase != ERogueStationTrainPhase::Unloading) continue;

            // Resolve current station entity
            if (!TrackSharedFragment.StationEntities.IsValidIndex(State.TargetStationIdx)) continue;			
            const FMassEntityHandle CurrentStationEntity = TrackSharedFragment.StationEntities[State.TargetStationIdx].Value;

			// Get station queue fragment
            FRogueStationQueueFragment* StationQueueFragment = EntityManager.GetFragmentDataPtr<FRogueStationQueueFragment>(CurrentStationEntity);
            if (!StationQueueFragment) continue;

            // Gather carriages for this engine
            const FMassEntityHandle EngineEntity = SubContext.GetEntity(i);
            const TArray<FMassEntityHandle>* CarriageList = TrainSubsystem->LeadToCarriages.Find(EngineEntity);
            if (!CarriageList) continue;

            // UNLOAD passengers whose Dest == current station (per carriage)
			if (State.StationTrainPhase == ERogueStationTrainPhase::Unloading)
			{
				int32 EmptyCarriages = 0;
				for (const FMassEntityHandle CarriageEntity : *CarriageList)
				{					
					auto* CarriageFragment = EntityManager.GetFragmentDataPtr<FRogueCarriageFragment>(CarriageEntity);
					if (!CarriageFragment) continue;

					if (CarriageFragment->Occupants.Num() <= 0)
					{
						EmptyCarriages++;
					}

					auto* CarriageTransformFragment = EntityManager.GetFragmentDataPtr<FTransformFragment>(CarriageEntity);
					if (!CarriageTransformFragment) continue;

					const FVector CarriageLocation = CarriageTransformFragment->GetTransform().GetLocation();

					if (CurrentTime >= CarriageFragment->NextAllowedUnloadTime)
					{
						const int32 NumOccupants = CarriageFragment->Occupants.Num();
						for (int32 Attempts = 0; Attempts < NumOccupants; ++Attempts)
						{
							const int32 Idx = CarriageFragment->UnloadCursor % CarriageFragment->Occupants.Num();
							const FMassEntityHandle Passenger = CarriageFragment->Occupants[Idx];

							if (!RoguePassengerUtility::IsHandleValid(EntityManager, Passenger))
							{
								CarriageFragment->Occupants.RemoveAtSwap(Idx);
								continue;
							}

							const FRoguePassengerFragment* PassengerFragment = EntityManager.GetFragmentDataPtr<FRoguePassengerFragment>(Passenger);
							if (!PassengerFragment)
							{
								CarriageFragment->Occupants.RemoveAtSwap(Idx);
								continue;
							}

							// Only disembark if this is the destination station
							if (PassengerFragment->DestinationStation == CurrentStationEntity)
							{
								RoguePassengerUtility::Disembark(EntityManager, SubContext, *CarriageFragment, Idx, CarriageLocation);
								CarriageFragment->NextAllowedUnloadTime = CurrentTime + Settings->UnloadIntervalSeconds;
								
								// Keeping UnloadCursor at same Idx; the next passenger shifts into this slot
								break;
							}

							// Advance cursor if this passenger is not for this station
							++CarriageFragment->UnloadCursor;
						}
					}					
				}

				if (EmptyCarriages >= CarriageList->Num())
				{
					// All carriages empty, skip to loading phase
					State.StationTrainPhase = ERogueStationTrainPhase::Loading;
				}
			}

            // LOAD passengers whose dest != current station (per carriage)
			if (State.StationTrainPhase == ERogueStationTrainPhase::Loading)
			{
				for (const FMassEntityHandle CarriageEntity : *CarriageList)
				{
					auto* CarriageFragment = EntityManager.GetFragmentDataPtr<FRogueCarriageFragment>(CarriageEntity);
					const FTransformFragment* CarriageTransformFragment = EntityManager.GetFragmentDataPtr<FTransformFragment>(CarriageEntity);
					if (!CarriageFragment || !CarriageTransformFragment) continue;

					const int32 FreeSlots = CarriageFragment->Capacity - CarriageFragment->Occupants.Num();
					if (FreeSlots <= 0) continue;

					int32 BoardingBudget = FMath::Min(FreeSlots, MaxLoadPerTickPerCar);
					if (BoardingBudget <= 0) continue;

					// Build a list of WP indices from the TMap
					TArray<int32> WaitingPointIndices;
					WaitingPointIndices.Reserve(StationQueueFragment->Grids.Num());
					for (const auto& Pair : StationQueueFragment->Grids)
					{
						WaitingPointIndices.Add(Pair.Key);
					}

					// Sort by distance to carriage
					const FVector CarriageLocation = CarriageTransformFragment->GetTransform().GetLocation();
					WaitingPointIndices.Sort([&](const int32 A, const int32 B)
					{
						const FVector& PositionA = StationQueueFragment->WaitingPoints[A];
						const FVector& PositionB = StationQueueFragment->WaitingPoints[B];
						return FVector::DistSquared(PositionA, CarriageLocation) < FVector::DistSquared(PositionB, CarriageLocation);
					});

					// Drain queues in waiting point distance order
					for (int32 j = 0; j < WaitingPointIndices.Num() && BoardingBudget > 0; ++j)
					{
						const int32 WaitingPointIdx = WaitingPointIndices[j];
		
						while (BoardingBudget > 0)
						{
							FMassEntityHandle Passenger;
							int32 SlotIdx = INDEX_NONE;
							FVector SlotPos;

							// Peek at next passenger in queue, if none move to next waiting point
							if (!RogueStationQueueUtility::PeekFromGrid(EntityManager, *StationQueueFragment, WaitingPointIdx, Passenger, CurrentStationEntity, SlotIdx, SlotPos))
								break;
							
							// Try to board passenger, if successful remove from queue, if unsuccessful break to next waiting point as carriage is likely full
							if (RoguePassengerUtility::TryBoard(EntityManager, SubContext, Passenger, CarriageEntity, *CarriageFragment))
							{
								// Successfully boarded — release the slot
								if (const FRoguePassengerFragment* PassengerFragment = EntityManager.GetFragmentDataPtr<FRoguePassengerFragment>(Passenger))
								{
									RogueStationQueueUtility::ReleaseSlot(*StationQueueFragment, *PassengerFragment);
								}
					
								// Clear passenger’s waiting data
								if (FRoguePassengerFragment* PassengerFragmentMutable = EntityManager.GetFragmentDataPtr<FRoguePassengerFragment>(Passenger))
								{
									PassengerFragmentMutable->WaitingPointIdx = INDEX_NONE;
									PassengerFragmentMutable->WaitingSlotIdx = INDEX_NONE;
									PassengerFragmentMutable->bWaiting = false;
								}
								--BoardingBudget;
							}
							else
							{
								// Carriage full or other issue, break to next waiting point
								break;
							}
							
						}
					}
				}
			}
        }
    });
}
