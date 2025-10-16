// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Processors/Trains/RogueTrainEngineMovementProcessor.h"

#include "MassCommonFragments.h"
#include "MassCommonTypes.h"
#include "MassExecutionContext.h"
#include "Data/RogueDeveloperSettings.h"
#include "Mass/Fragments/RogueFragments.h"
#include "Subsystems/RogueTrainWorldSubsystem.h"
#include "Utilities/RogueTrainUtility.h"

URogueTrainEngineMovementProcessor::URogueTrainEngineMovementProcessor() : EntityQuery(*this)
{	
	ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::AllNetModes);
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Movement;
}

void URogueTrainEngineMovementProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FRogueSplineFollowFragment>(EMassFragmentAccess::ReadWrite, EMassFragmentPresence::All);
	EntityQuery.AddRequirement<FRogueTrainStateFragment>(EMassFragmentAccess::ReadWrite, EMassFragmentPresence::All);	
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite, EMassFragmentPresence::All);
	EntityQuery.AddTagRequirement<FRogueTrainEngineTag>(EMassFragmentPresence::All);
	EntityQuery.RegisterWithProcessor(*this);	
}

void URogueTrainEngineMovementProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	auto* TrainSubsystem = Context.GetWorld()->GetSubsystem<URogueTrainWorldSubsystem>();
	if (!TrainSubsystem) return;

	const FRogueTrackSharedFragment& TrackSharedFragment = TrainSubsystem->GetTrackShared();
	if (!TrackSharedFragment.IsValid()) return;

	const auto* Settings = GetDefault<URogueDeveloperSettings>();
	if (!Settings) return;

	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& SubContext)
	{
		const auto FollowView = SubContext.GetMutableFragmentView<FRogueSplineFollowFragment>();
		const auto StateView  = SubContext.GetMutableFragmentView<FRogueTrainStateFragment>();
		const auto TransformView = SubContext.GetMutableFragmentView<FTransformFragment>();

		for (int32 i = 0; i < SubContext.GetNumEntities(); ++i)
		{
			auto& Follow = FollowView[i];
			const auto& State  = StateView[i];

			const float TargetSpeed = State.bIsStopping ? Settings->StationApproachSpeed : Settings->LeadCruiseSpeed;

			if (!State.bAtStation)
			{
				Follow.Speed = TargetSpeed;
				const float DeltaTime = (Follow.Speed * SubContext.GetDeltaTimeSeconds()) / TrackSharedFragment.TrackLength;
				Follow.Alpha = RogueTrainUtility::WrapTrackAlpha(Follow.Alpha + DeltaTime);
			}
			else
			{
				Follow.Speed = 0.f;
			}

			RogueTrainUtility::SampleSpline(TrackSharedFragment, Follow.Alpha, Follow.WorldPos, Follow.WorldFwd);
			
			// Build an orientation (forward = tangent, up = world up or spline up if you have it)
			const FVector Fwd = Follow.WorldFwd.GetSafeNormal();
			const FQuat Rot = FRotationMatrix::MakeFromXZ(Fwd, FVector::UpVector).ToQuat(); // could add TrackSharedFragment.Up track up etc

			// Write to the transform fragment
			FTransform& TrainTransform = TransformView[i].GetMutableTransform();
			TrainTransform.SetLocation(Follow.WorldPos);
			TrainTransform.SetRotation(Rot);
		}
	});
}
