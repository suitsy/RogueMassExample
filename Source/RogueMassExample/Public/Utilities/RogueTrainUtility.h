// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Mass/Fragments/RogueFragments.h"


namespace RogueTrainUtility
{
	inline float WrapTrackAlpha(const float Alpha) { return Alpha - FMath::FloorToFloat(Alpha); }
	int32 FindNextStation(const USplineComponent& Spline,const TArray<FRoguePlatformData>& Platforms, const float CurrentAlpha);
	float AlphaAtWorld(const USplineComponent& Spline, const FVector& WorldPos);
	float ArcDistanceWrapped(const float FromAlpha, const float ToAlpha);
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
	bool GetStationSplineSample(
		const FRogueTrackSharedFragment& Track,
		const float StationTrackAlpha,
		const float AlongOffsetCm,
		const float LateralOffsetCm,
		const float VerticalOffsetCm,
		FSplineStationSample& Out);

	/** Convenience overload: zero offsets */
	inline bool GetStationSplineSample(const FRogueTrackSharedFragment& TrackSharedFragment, const float StationTrackAlpha, FSplineStationSample& Out)
	{
		return GetStationSplineSample(TrackSharedFragment, StationTrackAlpha, /*Along*/0.f, /*Lat*/0.f, /*Z*/0.f, Out);
	}

	FTransform SampleTrackFrame(const USplineComponent& Spline, float Alpha);
	FVector SampleDockPoint(const USplineComponent& Spline, float Alpha);
	void BuildPlatformSegment(const USplineComponent& Spline, const FRogueStationConfig& StationConfigData, FRoguePlatformData& Out);
	void ComputeConsistPlacement(const FRogueTrackSharedFragment& Track, const float EngineHeadAlpha, const int32 NumCarriages, TArray<FRoguePlacedCar>& Out);
}
