// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Processors/Creators/RogueTrainCreatorProcessor.h"
#include "Subsystems/RogueTrainWorldSubsystem.h"
#include "MassEntitySubsystem.h"
#include "MassCommonTypes.h"
#include "MassEntityConfigAsset.h"
#include "Data/RogueDeveloperSettings.h"
#include "Mass/Processors/Creators/RogueTrainStationCreatorProcessor.h"
#include "Utilities/RogueTrainUtility.h"

URogueTrainCreatorProcessor::URogueTrainCreatorProcessor()
{
	bAutoRegisterWithProcessingPhases = true;
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Tasks;
	ExecutionOrder.ExecuteAfter.Add(URogueTrainStationCreatorProcessor::StaticClass()->GetFName());
}

void URogueTrainCreatorProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	if (bCreated) return;
	
	const UWorld* World = GetWorld();
	if (!World) return;
	const auto* Settings = GetDefault<URogueDeveloperSettings>();
	if (!World || !Settings || !Settings->TrainEngineConfig || !Settings->TrainCarriageConfig) return;

	auto* TrainSubsystem = World->GetSubsystem<URogueTrainWorldSubsystem>();
	if (!TrainSubsystem) return;
	TWeakObjectPtr<URogueTrainWorldSubsystem> TrainSubsystemWeak = TrainSubsystem;

	const FRogueTrackSharedFragment& TrackSharedFragment = TrainSubsystem->GetTrackShared();
	if (!TrackSharedFragment.IsValid()) return;

	const int32 NumStations = TrackSharedFragment.StationTrackAlphas.Num();
	if (NumStations <= 0) return;
	
	const int32 NumberOfTrains = Settings->NumTrains;	
	const int32 CarriagesPer = Settings->CarriagesPerTrain;
	const float SpacingMeters = Settings->CarriageSpacingMeters;
	const int32 CapacityPerCar = Settings->MaxPassengersPerCarriage;
	const int32 Passes = FMath::DivideAndRoundUp(NumberOfTrains, NumStations);
	
	for (int i = 0; i < NumberOfTrains; ++i)
	{
		const int32 StationIdx = i % NumStations;
		const int32 PassIdx = i / NumStations;
		const int32 NextIdx = (StationIdx + 1) % NumStations;
		const float T0 = TrackSharedFragment.StationTrackAlphas[StationIdx];
		const float T1 = TrackSharedFragment.StationTrackAlphas[NextIdx];
		const float dT = RogueTrainUtility::ArcDistanceWrapped(T0, T1);  // segment length in normalized
		const float Frac = (Passes <= 1) ? 0.f : static_cast<float>(PassIdx) / static_cast<float>(Passes); // 0 (at station), 0.5 (mid), 0.333 etc.
		const float TrainAlpha = RogueTrainUtility::WrapTrackAlpha(T0 + dT * Frac);

		RogueTrainUtility::FSplineStationSample Sample;
		if (!RogueTrainUtility::GetStationSplineSample(TrackSharedFragment, TrainAlpha, Sample))
		{
			/*Along*/ //0.f,        // e.g. +100.f to place a bit ahead
			/*Lateral*/ //0.f,      // e.g. +150.f to offset to platform side
			/*Vertical*/ //0.f,     // e.g. +5.f lift
			continue;
		}
		
		const FMassEntityTemplate& TrainEngineTemplate = Settings->TrainEngineConfig->GetOrCreateEntityTemplate(*GetWorld());
		const FMassEntityTemplate& TrainCarriageTemplate = Settings->TrainCarriageConfig->GetOrCreateEntityTemplate(*GetWorld());
		if (!TrainEngineTemplate.IsValid() || !TrainCarriageTemplate.IsValid()) continue;			
			
		FRogueSpawnRequest Request;
		Request.Type = ERogueEntityType::TrainEngine;
		Request.EntityTemplate = TrainEngineTemplate;
		Request.RemainingCount = 1;
		Request.SpawnLocation = Sample.Location;
		Request.StartAlpha = TrainAlpha; 
		Request.StationIndex = StationIdx;

		Request.OnSpawned = [TrainAlpha, CarriagesPer, SpacingMeters, CapacityPerCar, TrainCarriageTemplate, TrainSubsystemWeak](const TArray<FMassEntityHandle>& Spawned)
		{
			auto* TrainSubsystemLocal = TrainSubsystemWeak.Get();
			if (Spawned.Num() == 0 || !TrainSubsystemLocal) return;
			const FMassEntityHandle LeadHandle = Spawned[0];

			const FRogueTrackSharedFragment& TrackSharedFragment = TrainSubsystemLocal->GetTrackShared();
			if (!TrackSharedFragment.IsValid()) return;

			for (int32 c = 1; c <= CarriagesPer; ++c)
			{
				// Position behind the engine and other carriages
				const float backDistCm = (c * SpacingMeters) * 100.f;
				const float dT = backDistCm / TrackSharedFragment.TrackLength;
				const float CarriageAlpha = RogueTrainUtility::WrapTrackAlpha(TrainAlpha - dT);

				RogueTrainUtility::FSplineStationSample CarriageSample;
				if (!RogueTrainUtility::GetStationSplineSample(TrackSharedFragment, CarriageAlpha, CarriageSample))
					continue;
				
				FRogueSpawnRequest CarReq;
				CarReq.Type				= ERogueEntityType::TrainCarriage;
				CarReq.EntityTemplate	= TrainCarriageTemplate;
				CarReq.RemainingCount	= 1;
				CarReq.LeadHandle		= LeadHandle;
				CarReq.CarriageIndex	= c;
				CarReq.SpacingMeters	= SpacingMeters;
				CarReq.CarriageCapacity	= CapacityPerCar;
				CarReq.StartAlpha		= CarriageAlpha;                 // sets FRogueSplineFollowFragment.Alpha on spawn
				CarReq.SpawnLocation	= CarriageSample.Location;   // optional: if your template has Transform

				TrainSubsystemLocal->EnqueueSpawns(CarReq);
			}
		};

		TrainSubsystem->EnqueueSpawns(Request);
	}

	bCreated = true;
}
