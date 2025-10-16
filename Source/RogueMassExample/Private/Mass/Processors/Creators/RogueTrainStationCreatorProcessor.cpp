// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Processors/Creators/RogueTrainStationCreatorProcessor.h"
#include "MassCommonTypes.h"
#include "MassEntityConfigAsset.h"
#include "MassEntityTemplate.h"
#include "Actors/RogueTrainStation.h"
#include "Data/RogueDeveloperSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Subsystems/RogueTrainWorldSubsystem.h"

URogueTrainStationCreatorProcessor::URogueTrainStationCreatorProcessor()
{
	bAutoRegisterWithProcessingPhases = true;	
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Tasks; 
}

void URogueTrainStationCreatorProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	if (bCreated) return;
	
	// One-time station spawn (authoring actors already placed)
	const UWorld* World = GetWorld();
	if (!World) return;
	
	auto* TrainSubsystem = World->GetSubsystem<URogueTrainWorldSubsystem>();
	if (!TrainSubsystem) return;

	const auto* Settings = GetDefault<URogueDeveloperSettings>();
	if (!World || !Settings || !Settings->StationConfig) return;

	TArray<AActor*> StationActors;
	UGameplayStatics::GetAllActorsOfClass(World, ARogueTrainStation::StaticClass(), StationActors);	

	for (int i = 0; i < StationActors.Num(); ++i)
	{
		if (!StationActors.IsValidIndex(i) || !StationActors[i]) continue;

		const AActor* StationActor = StationActors[i];		
		const FMassEntityTemplate& StationTemplate = Settings->StationConfig->GetOrCreateEntityTemplate(*GetWorld());
		if (StationTemplate.IsValid())
		{
			FRogueSpawnRequest Request;
			Request.Type = ERogueEntityType::Station;
			Request.EntityTemplate = StationTemplate;
			Request.RemainingCount = 1;
			Request.SpawnLocation = StationActor->GetActorLocation();   // your computed platform anchor
			Request.StationIndex = i;

			TrainSubsystem->EnqueueSpawns(Request);
		}		
	}

	bCreated = true;
}
