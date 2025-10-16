// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SplineComponent.h"
#include "Mass/Fragments/RogueFragments.h"

namespace RogueTrainUtility
{
	inline float WrapTrackAlpha(const float Alpha)
	{		
		return Alpha - FMath::FloorToFloat(Alpha);
	}

	inline int32 FindNextStation(const TArray<float>& StationAlphas, const float CurrentAlpha)
	{
		if (StationAlphas.Num() == 0) return INDEX_NONE;
		for (int32 i = 0; i < StationAlphas.Num(); ++i)
		{
			if (StationAlphas[i] >= CurrentAlpha) return i;
		}
		
		return 0; 
	}

	inline float ArcDistanceWrapped(const float FromAlpha, const float ToAlpha)
	{
		float d = ToAlpha - FromAlpha;
		if (d < 0.f) d += 1.f;
		return d; 
	}

	void SampleSpline(const FRogueTrackSharedFragment& TrackSharedFragment, const float Alpha, FVector& OutPos, FVector& OutFwd);

	struct FSplineStationSample
	{
		FVector   Location = FVector::ZeroVector;
		FVector   Forward  = FVector::ForwardVector;
		FVector   Right    = FVector::RightVector;
		FVector   Up       = FVector::UpVector;
		FTransform World   = FTransform::Identity;
		float     Distance = 0.f;   // cm along spline
		float     Alpha        = 0.f;   // normalized [0..1]
	};

	/** Returns true if sampled successfully.
	 *  @param Track			Track fragment with spline reference
	 *  @param StationTrackAlpha         Normalized station T in [0..1]
	 *  @param AlongOffsetCm    Extra distance along the spline in cm (positive moves forward)
	 *  @param LateralOffsetCm  Offset to the right of the track (platform side) in cm
	 *  @param VerticalOffsetCm Vertical offset in cm
	 *  @param Out				Spline sample output
	 */
	inline bool GetStationSplineSample(
		const FRogueTrackSharedFragment& Track,
		const float StationTrackAlpha,
		const float AlongOffsetCm,
		const float LateralOffsetCm,
		const float VerticalOffsetCm,
		FSplineStationSample& Out)
	{
		const USplineComponent* Spline = Track.Spline.Get();
		if (!Spline) return false;

		const float Len = FMath::Max(1.f, Track.TrackLength);
		const float RawDist = StationTrackAlpha * Len + AlongOffsetCm;
		float Dist = FMath::Fmod(RawDist, Len); // wrap to [0, Len)
		if (Dist < 0.f) Dist += Len;                // negative wrap

		// Grab full transform at distance (world space)
		const FTransform Sx = Spline->GetTransformAtDistanceAlongSpline(Dist, ESplineCoordinateSpace::World);

		Out.Distance = Dist;
		Out.Alpha = (Len > 0.f) ? (Dist / Len) : 0.f;

		// Basis
		const FRotator R = Sx.Rotator();
		const FQuat    Q = Sx.GetRotation();
		const FVector  Fwd = Q.GetForwardVector(); // along track
		const FVector  Right = Q.GetRightVector(); // lateral
		const FVector  Up = Q.GetUpVector();       // vertical

		// Offsets
		FVector L = Sx.GetLocation();
		L += Right * LateralOffsetCm;
		L += Up    * VerticalOffsetCm;

		Out.Location = L;
		Out.Forward  = Fwd.GetSafeNormal();
		Out.Right    = Right.GetSafeNormal();
		Out.Up       = Up.GetSafeNormal();
		Out.World    = FTransform(Q, L, Sx.GetScale3D());
		
		return true;
	}

	/** Convenience overload: zero offsets */
	inline bool GetStationSplineSample(const FRogueTrackSharedFragment& TrackSharedFragment, const float StationTrackAlpha, FSplineStationSample& Out)
	{
		return GetStationSplineSample(TrackSharedFragment, StationTrackAlpha, /*Along*/0.f, /*Lat*/0.f, /*Z*/0.f, Out);
	}
}
