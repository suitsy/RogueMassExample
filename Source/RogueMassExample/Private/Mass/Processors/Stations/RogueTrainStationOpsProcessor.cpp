// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Processors/Stations/RogueTrainStationOpsProcessor.h"
#include "MassCommonTypes.h"
#include "MassExecutionContext.h"
#include "Data/RogueDeveloperSettings.h"
#include "Utilities/RogueTrainUtility.h"


URogueTrainStationOpsProcessor::URogueTrainStationOpsProcessor()/*: EntityQuery(*this)*/
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::AllNetModes);
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
	ExecutionOrder.ExecuteBefore.Add(UE::Mass::ProcessorGroupNames::Avoidance);
}

void URogueTrainStationOpsProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	/*EntityQuery.AddRequirement<FRogueTrainStateFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FRogueSplineFollowFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddTagRequirement<FRogueTrainEngineTag>(EMassFragmentPresence::All);
	EntityQuery.RegisterWithProcessor(*this);*/
}

void URogueTrainStationOpsProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	const auto* Settings = GetDefault<URogueDeveloperSettings>();
	const float MaxDwell = Settings ? Settings->MaxDwellTimeSeconds : 12.f;

	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& SubContext)
	{
		const auto StateView  = SubContext.GetMutableFragmentView<FRogueTrainStateFragment>();

		for (int32 i = 0; i < SubContext.GetNumEntities(); ++i)
		{
			auto& State = StateView[i];
			if (!State.bAtStation) continue;

			State.DwellTimer += SubContext.GetDeltaTimeSeconds();

			// TODO: Unload passengers destined for State.TargetStationIndex
			// TODO: Load passengers from station queues into carriages while capacity

			const bool bTimeUp = State.DwellTimer >= MaxDwell;
			if (bTimeUp /* || optional: queues empty and all processed */)
			{
				State.bAtStation = false;
				State.bIsStopping = false;
				State.DwellTimer = 0.f;
			}
		}
	});
}
