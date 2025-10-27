// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Processors/Trains/RogueTrainHeadwayProcessor.h"
#include "MassCommonTypes.h"
#include "MassExecutionContext.h"
#include "Data/RogueDeveloperSettings.h"
#include "Mass/Fragments/RogueFragments.h"
#include "Subsystems/RogueTrainWorldSubsystem.h"
#include "Utilities/RogueTrainUtility.h"

URogueTrainHeadwayProcessor::URogueTrainHeadwayProcessor(): EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::AllNetModes);
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Movement;
}

void URogueTrainHeadwayProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FRogueTrainTrackFollowFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FRogueTrainStateFragment>( EMassFragmentAccess::ReadWrite);
	EntityQuery.AddTagRequirement<FRogueTrainEngineTag>(EMassFragmentPresence::All);
	EntityQuery.RegisterWithProcessor(*this);
}

void URogueTrainHeadwayProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	auto* TrainSubsystem = Context.GetWorld()->GetSubsystem<URogueTrainWorldSubsystem>();
	if (!TrainSubsystem) return;

	const FRogueTrackSharedFragment& TrackSharedFragment = TrainSubsystem->GetTrackShared();
	if (!TrackSharedFragment.IsValid()) return;

	const auto* Settings = GetDefault<URogueDeveloperSettings>();
	if (!Settings) return;

	const float TrackLength = TrackSharedFragment.TrackLength;
	const float MinGap = Settings ? Settings->MinHeadway : 1500.f;
	const float FullGap = MinGap * 2.f;
	const float Spacing = (Settings ? Settings->CarriageSpacingMeters : 8.f) * 100.f;	

	if(TrackLength <= 0.f) return;

	struct FEntry { FMassEntityHandle EntityHandle; float LeadAlpha; float TailAlpha; };
	TArray<FEntry> Engines; Engines.Reserve(64);

	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& SubContext)
	{
		const TConstArrayView<FRogueTrainTrackFollowFragment> FollowView = SubContext.GetFragmentView<FRogueTrainTrackFollowFragment>();
		const TArrayView<FRogueTrainStateFragment> StateView = SubContext.GetMutableFragmentView<FRogueTrainStateFragment>();	

		for (int32 i = 0; i < SubContext.GetNumEntities(); ++i)
		{
			const auto& Follow = FollowView[i];
			auto& State = StateView[i];

			// Clear headway
			State.HeadwaySpeedScale = 1.f;

			const float LeadAlpha = Follow.Alpha;

			// per-lead carriages if you track it, else default
			int32 NumCars = Settings ? Settings->CarriagesPerTrain : 3;
			if (TrainSubsystem->CarriageCounts.Contains(SubContext.GetEntity(i)))
			{
				NumCars = TrainSubsystem->CarriageCounts[SubContext.GetEntity(i)];
			}
			
			const float TrainLength = NumCars * Spacing;
			const float DeltaTime = TrainLength / TrackLength;
			const float TailAlpha = RogueTrainUtility::WrapTrackAlpha(LeadAlpha - DeltaTime);

			Engines.Add({ SubContext.GetEntity(i), LeadAlpha, TailAlpha });
		}
	});

	if (Engines.Num() <= 1) return;

	Algo::SortBy(Engines, &FEntry::LeadAlpha);		

	auto GapToScale = [&](const float Gap)
	{
		const float t = FMath::Clamp((Gap - MinGap) / (FullGap - MinGap), 0.f, 1.f);
		return t * t * (3.f - 2.f * t);
	};

	// Check gap to next engine's tail
	for (int32 Idx = 0; Idx < Engines.Num(); ++Idx)
	{
		const FEntry& Current = Engines[Idx];
		const FEntry& Next = Engines[(Idx+1) % Engines.Num()];

		// Distance forward along track from current.lead to next.tail
		const float ArcDist = RogueTrainUtility::ArcDistanceWrapped(Current.LeadAlpha, Next.TailAlpha);
		const float Gap = ArcDist * TrackLength;

		if (auto* State = EntityManager.GetFragmentDataPtr<FRogueTrainStateFragment>(Current.EntityHandle))
		{
			const float Scale = GapToScale(Gap);
			State->HeadwaySpeedScale = FMath::Min(State->HeadwaySpeedScale, Scale);
		}
	}
}
