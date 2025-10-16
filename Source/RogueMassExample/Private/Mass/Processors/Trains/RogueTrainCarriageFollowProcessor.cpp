// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Processors/Trains/RogueTrainCarriageFollowProcessor.h"

#include "MassCommonFragments.h"
#include "MassCommonTypes.h"
#include "MassEntityView.h"
#include "MassExecutionContext.h"
#include "Data/RogueDeveloperSettings.h"
#include "Mass/Processors/Trains/RogueTrainEngineMovementProcessor.h"
#include "Subsystems/RogueTrainWorldSubsystem.h"
#include "Utilities/RogueTrainUtility.h"

URogueTrainCarriageFollowProcessor::URogueTrainCarriageFollowProcessor() : EntityQuery(*this)
{
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::AllNetModes);
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Movement;	
	ExecutionOrder.ExecuteAfter.Add(URogueTrainEngineMovementProcessor::StaticClass()->GetFName());
}

void URogueTrainCarriageFollowProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FRogueSplineFollowFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite, EMassFragmentPresence::All);
	EntityQuery.AddRequirement<FRogueTrainLinkFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddTagRequirement<FRogueTrainCarriageTag>(EMassFragmentPresence::All);
	EntityQuery.RegisterWithProcessor(*this);
}

void URogueTrainCarriageFollowProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	auto* TrainSubsystem = Context.GetWorld()->GetSubsystem<URogueTrainWorldSubsystem>();
	if (!TrainSubsystem) return;

	const FRogueTrackSharedFragment& TrackSharedFragment = TrainSubsystem->GetTrackShared();
	if (!TrackSharedFragment.IsValid()) return;

	const auto* Settings = GetDefault<URogueDeveloperSettings>();
	const float SpacingM   = Settings ? Settings->CarriageSpacingMeters : 8.f;
	const float SpacingCm  = SpacingM * 100.f;

	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& SubContext)
	{
		const auto FollowView = SubContext.GetMutableFragmentView<FRogueSplineFollowFragment>();
		const auto LinkView = SubContext.GetFragmentView<FRogueTrainLinkFragment>();
		const auto TransformView = SubContext.GetMutableFragmentView<FTransformFragment>();

		for (int32 i = 0; i < SubContext.GetNumEntities(); ++i)
		{
			const auto& Link = LinkView[i];
			if (!Link.LeadHandle.IsSet() || !EntityManager.IsEntityValid(Link.LeadHandle))
				continue;
	
			const FRogueSplineFollowFragment* LeadFollow = EntityManager.GetFragmentDataPtr<FRogueSplineFollowFragment>(Link.LeadHandle);
			if (!LeadFollow) continue;			

			RogueTrainUtility::FSplineStationSample SplineSample;
			const float OffsetDistCm = FMath::Max(1, Link.CarriageIndex) * SpacingCm;
			if (!RogueTrainUtility::GetStationSplineSample(TrackSharedFragment, LeadFollow->Alpha, -OffsetDistCm, 0.f, 0.f, SplineSample))
				continue;

			// Update carriage follow state
			auto& Follow = FollowView[i];
			Follow.Alpha = SplineSample.Alpha;
			Follow.WorldPos = SplineSample.Location;
			Follow.WorldFwd = SplineSample.Forward;
			
			FTransform& TrainTransform = TransformView[i].GetMutableTransform();
			TrainTransform = SplineSample.World;
			const FQuat Rot = FRotationMatrix::MakeFromXZ(SplineSample.Forward, FVector::UpVector).ToQuat();
			TrainTransform.SetRotation(Rot);
		}
	});
}