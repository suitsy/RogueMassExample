// Fill out your copyright notice in the Description page of Project Settings.


#include "Utilities/RogueTrainUtility.h"
#include "Components/SplineComponent.h"

using namespace RogueTrainUtility;

void RogueTrainUtility::SampleSpline(const FRogueTrackSharedFragment& TrackSharedFragment, const float Alpha, FVector& OutPos, FVector& OutFwd)
{
	if (const USplineComponent* Spline = TrackSharedFragment.Spline.Get())
	{
		const float Distance = Alpha * TrackSharedFragment.TrackLength;
		OutPos = Spline->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
		const FVector Tangent = Spline->GetDirectionAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
		OutFwd = Tangent.GetSafeNormal();
	}
}

