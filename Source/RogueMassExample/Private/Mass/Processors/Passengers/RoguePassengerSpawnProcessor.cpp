// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Processors/Passengers/RoguePassengerSpawnProcessor.h"
#include "MassCommonTypes.h"
#include "MassExecutionContext.h"
#include "Data/RogueDeveloperSettings.h"
#include "Subsystems/RogueTrainWorldSubsystem.h"


URoguePassengerSpawnProcessor::URoguePassengerSpawnProcessor(): EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ProcessingPhase = EMassProcessingPhase::FrameEnd;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Tasks;
}

void URoguePassengerSpawnProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FRogueStationQueueFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FRogueStationFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddTagRequirement<FRogueTrainStationTag>(EMassFragmentPresence::All);
	EntityQuery.RegisterWithProcessor(*this);
}

void URoguePassengerSpawnProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	const auto* Settings = GetDefault<URogueDeveloperSettings>();
	if (!Settings) return;

	auto* TrainSubsystem = Context.GetWorld()->GetSubsystem<URogueTrainWorldSubsystem>();
	if (!TrainSubsystem) return;
	
	const FMassEntityTemplate* PassengerEntityTemplate = TrainSubsystem->GetPassengerTemplate();
	if (!PassengerEntityTemplate->IsValid()) return;
	
	SpawnAccumulator += Context.GetDeltaTimeSeconds();
	if (SpawnAccumulator < Settings->SpawnIntervalSeconds) return;
	SpawnAccumulator = 0.f;

	// Cap overall passengers
	if (TrainSubsystem->GetLiveCount(ERogueEntityType::Passenger) >= Settings->MaxPassengersOverall) return;

	// Generate list of station entities (so we can pick one randomly)
	TArray<FMassEntityHandle> StationEntities;
	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& SubContext)
	{
		for (int32 i = 0; i < SubContext.GetNumEntities(); ++i)
		{
			StationEntities.Add(SubContext.GetEntity(i));
		}
	});

	if (StationEntities.Num() < 2) return;

	// Pick a random station that has spawn points to spawn at
	const FMassEntityHandle StationHandle = StationEntities[FMath::RandHelper(StationEntities.Num())];

	// Get station queue fragment from chosen station
	auto* StationQueueFragment = EntityManager.GetFragmentDataPtr<FRogueStationQueueFragment>(StationHandle);
	if (!StationQueueFragment || StationQueueFragment->SpawnPoints.Num() == 0) return;

	// Get a random station index for destination that is not current station index
	const FMassEntityHandle OriginStation = StationHandle;
	FMassEntityHandle DestinationStation = OriginStation;
	while (DestinationStation == OriginStation)
	{
		const int32 DestIdx = FMath::RandHelper(StationEntities.Num());
		if (StationEntities.IsValidIndex(DestIdx))
		{
			DestinationStation = StationEntities[DestIdx];
		}		
	}

	if (!DestinationStation.IsValid()) return;
	
	// Choose a random waiting point
	const int32 WaitingIdx = (StationQueueFragment->WaitingPoints.Num() > 0)
		? FMath::RandRange(0, StationQueueFragment->WaitingPoints.Num() - 1)
		: INDEX_NONE;

	// Choose a random spawn point
	const FVector SpawnLoc = StationQueueFragment->SpawnPoints[FMath::RandHelper(StationQueueFragment->SpawnPoints.Num())];

	FRogueSpawnRequest Request;
	Request.Type				= ERogueEntityType::Passenger;
	Request.EntityTemplate		= PassengerEntityTemplate;
	Request.RemainingCount		= 1;
	Request.Transform			= FTransform(SpawnLoc);
	Request.OriginStation		= OriginStation;
	Request.DestinationStation  = DestinationStation;
	Request.WaitingPointIdx		= WaitingIdx;
	Request.AcceptanceRadius	= Settings->PassengerAcceptanceRadius;
	Request.MaxSpeed			= Settings->PassengerMaxSpeed;

	TrainSubsystem->EnqueueSpawns(Request);
}
