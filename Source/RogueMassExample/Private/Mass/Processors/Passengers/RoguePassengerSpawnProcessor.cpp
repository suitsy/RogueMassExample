// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Processors/Passengers/RoguePassengerSpawnProcessor.h"
#include "MassCommonTypes.h"
#include "MassEntityConfigAsset.h"
#include "MassExecutionContext.h"
#include "Data/RogueDeveloperSettings.h"
#include "Subsystems/RogueTrainWorldSubsystem.h"
#include "Utilities/RogueTrainUtility.h"


URoguePassengerSpawnProcessor::URoguePassengerSpawnProcessor()
{
	bAutoRegisterWithProcessingPhases = true;
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ProcessingPhase = EMassProcessingPhase::FrameEnd;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Tasks;
}

void URoguePassengerSpawnProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	/*EntityQuery.AddRequirement<FRogueStationQueueFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FRogueStationRefFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddTagRequirement<FRogueTrainStationTag>(EMassFragmentPresence::All);
	EntityQuery.RegisterWithProcessor(*this);*/
}

void URoguePassengerSpawnProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	const auto* Settings = GetDefault<URogueDeveloperSettings>();
	if (!Settings) return;

	SpawnAccumulator += Context.GetDeltaTimeSeconds();
	if (SpawnAccumulator < Settings->SpawnIntervalSeconds) return;
	SpawnAccumulator = 0.f;

	// Cap overall passengers
	auto* TrainSubsystem = Context.GetWorld()->GetSubsystem<URogueTrainWorldSubsystem>();
	if (!TrainSubsystem) return;
	if (TrainSubsystem->GetLiveCount(ERogueEntityType::Passenger) >= Settings->MaxPassengersOverall) return;

	// Pick a random station that has spawn points
	TArray<FMassEntityHandle> StationEntities;
	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& SubContext)
	{
		for (int32 i = 0; i < SubContext.GetNumEntities(); ++i)
		{
			StationEntities.Add(SubContext.GetEntity(i));
		}
	});

	if (StationEntities.Num() == 0) return;

	// Choose a random station with spawn points
	const FMassEntityHandle StationHandle = StationEntities[FMath::RandHelper(StationEntities.Num())];
	auto* StationQueueFragment = EntityManager.GetFragmentDataPtr<FRogueStationQueueFragment>(StationHandle);
	auto* StationRefFragment = EntityManager.GetFragmentDataPtr<FRogueStationRefFragment>(StationHandle);
	if (!StationQueueFragment || !StationRefFragment || StationQueueFragment->SpawnPoints.Num() == 0) return;

	// station count (for dest != origin)
	const int32 NumStations = TrainSubsystem->GetStations().Num();
	if (NumStations < 2) return;

	// Get a random station index for destination that is not current station index
	const int32 OriginIdx = StationRefFragment->StationIndex;
	int32 DestIdx = OriginIdx;
	while (DestIdx == OriginIdx)
	{
		DestIdx = FMath::RandRange(0, NumStations - 1);
	}

	// Choose a random waiting point
	const int32 WaitingIdx = (StationQueueFragment->WaitingPoints.Num() > 0)
		? FMath::RandRange(0, StationQueueFragment->WaitingPoints.Num() - 1)
		: INDEX_NONE;

	// Choose a random spawn point
	const FVector SpawnLoc = StationQueueFragment->SpawnPoints[FMath::RandHelper(StationQueueFragment->SpawnPoints.Num())];

	// Build spawn request 
	const FMassEntityTemplate& PassengerTemplate = Settings->PassengerConfig->GetOrCreateEntityTemplate(*Context.GetWorld());
	if (!PassengerTemplate.IsValid()) return;

	FRogueSpawnRequest Request;
	Request.Type               = ERogueEntityType::Passenger;
	Request.EntityTemplate     = PassengerTemplate;
	Request.RemainingCount     = 1;
	Request.SpawnLocation      = SpawnLoc;
	Request.OriginStationIndex = OriginIdx;
	Request.DestStationIndex   = DestIdx;
	Request.WaitingPointIdx    = WaitingIdx;

	TrainSubsystem->EnqueueSpawns(Request);
}
