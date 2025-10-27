// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Processors/Stations/RogueTrainStationDetectProcessor.h"
#include "MassCommonTypes.h"
#include "MassExecutionContext.h"
#include "Data/RogueDeveloperSettings.h"
#include "Mass/Fragments/RogueFragments.h"
#include "Subsystems/RogueTrainWorldSubsystem.h"
#include "Utilities/RogueTrainUtility.h"

URogueTrainStationDetectProcessor::URogueTrainStationDetectProcessor(): EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::AllNetModes);
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
	ExecutionOrder.ExecuteBefore.Add(UE::Mass::ProcessorGroupNames::Avoidance);
}

void URogueTrainStationDetectProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FRogueTrainTrackFollowFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FRogueTrainStateFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddTagRequirement<FRogueTrainEngineTag>(EMassFragmentPresence::All);
	EntityQuery.RegisterWithProcessor(*this);
}

void URogueTrainStationDetectProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	auto* TrainSubsystem = Context.GetWorld()->GetSubsystem<URogueTrainWorldSubsystem>();
	if (!TrainSubsystem) return;
	
	const FRogueTrackSharedFragment& TrackSharedFragment = TrainSubsystem->GetTrackShared();
	if (!TrackSharedFragment.IsValid()) return;

	const auto* Settings = GetDefault<URogueDeveloperSettings>();
	if(!Settings) return;
	
	const float StopRadius = Settings ? Settings->StationStopRadius : 600.f;
	const float ArriveRadius = Settings ? Settings->StationArrivalRadius : 50.f;

	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& SubContext)
	{
		const auto FollowView = SubContext.GetMutableFragmentView<FRogueTrainTrackFollowFragment>();
		const auto StateView  = SubContext.GetMutableFragmentView<FRogueTrainStateFragment>();

		for (int32 i = 0; i < SubContext.GetNumEntities(); ++i)
		{
			const auto& Follow = FollowView[i];
			auto& State = StateView[i];

			if (State.TargetStationIdx == INDEX_NONE)
			{
				State.TargetStationIdx = RogueTrainUtility::FindNextStation(TrackSharedFragment.StationTrackAlphas, Follow.Alpha);
				State.PrevAlpha = Follow.Alpha;
				continue; // next tick we’ll evaluate distance
			}

			//const float StationTrackAlpha = TrackSharedFragment.StationTrackAlphas[State.TargetStationIdx];
			const float DockAlpha = TrackSharedFragment.Platforms[State.TargetStationIdx].DockAlpha;
			const float Arc = RogueTrainUtility::ArcDistanceWrapped(Follow.Alpha, DockAlpha);
			const float Dist = Arc * TrackSharedFragment.TrackLength;
			const float DeltaTime = SubContext.GetDeltaTimeSeconds();
			
			if (!State.bAtStation)
			{
				// Approach band
				State.bIsStopping = (Dist <= StopRadius);

				// Enter dwell
				if (Dist <= ArriveRadius)
				{
					State.bAtStation = true;
					State.StationTimeRemaining = Settings ? Settings->MaxDwellTimeSeconds : 2.f;
				}
			}
			else
			{
				// Dwell countdown
				State.bIsStopping = true; // keep slowed/stopped while dwelling
				State.StationTimeRemaining -= DeltaTime;
				if (State.StationTimeRemaining <= 0.f)
				{
					// Depart now: retarget to NEXT station and leave
					State.bAtStation = false;
					State.StationTrainPhase = ERogueStationTrainPhase::NotStopped;
					State.bIsStopping = false;
					State.PreviousStationIndex = State.TargetStationIdx;
					State.TargetStationIdx = (State.TargetStationIdx + 1) % TrackSharedFragment.StationTrackAlphas.Num();
				}
			}
		}
	});	
}
