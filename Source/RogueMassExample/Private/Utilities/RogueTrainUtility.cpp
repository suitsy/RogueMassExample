// Fill out your copyright notice in the Description page of Project Settings.


#include "Utilities/RogueTrainUtility.h"
#include "Components/SplineComponent.h"
#include "Data/RogueDeveloperSettings.h"

using namespace RogueTrainUtility;

int32 RogueTrainUtility::FindNextStation(const USplineComponent& Spline,const TArray<FRoguePlatformData>& Platforms, const float CurrentAlpha)
{
	int32 BestIdx = INDEX_NONE;
	float BestArc = FLT_MAX;

	for (int32 i = 0; i < Platforms.Num(); ++i)
	{
		const FVector Ref = Platforms[i].Center; // or Dock position if you have one
		const float StationAlpha = AlphaAtWorld(Spline, Ref);

		const float Arc = ArcDistanceWrapped(CurrentAlpha, StationAlpha);
		if (Arc > KINDA_SMALL_NUMBER && Arc < BestArc) // ignore “at current alpha”
		{
			BestArc = Arc;
			BestIdx = i;
		}
	}
	return BestIdx; 
}

float RogueTrainUtility::AlphaAtWorld(const USplineComponent& Spline, const FVector& WorldPos)
{
	const float Len  = FMath::Max(1.f, Spline.GetSplineLength());
	const float Key  = Spline.FindInputKeyClosestToWorldLocation(WorldPos);
	const float Dist = Spline.GetDistanceAlongSplineAtSplineInputKey(Key);
	return FMath::Frac(Dist / Len);
}

float RogueTrainUtility::ArcDistanceWrapped(const float FromAlpha, const float ToAlpha)
{
	float d = ToAlpha - FromAlpha;
	if (d < 0.f) d += 1.f;
	return d; 
}

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

bool RogueTrainUtility::GetStationSplineSample(const FRogueTrackSharedFragment& Track, const float StationTrackAlpha,
	const float AlongOffsetCm, const float LateralOffsetCm, const float VerticalOffsetCm, FSplineStationSample& Out)
{
	const USplineComponent* Spline = Track.Spline.Get();
	if (!Spline) return false;

	const float Len = FMath::Max(1.f, Track.TrackLength);
	const float RawDist = StationTrackAlpha * Len + AlongOffsetCm;
	float Dist = FMath::Fmod(RawDist, Len); // wrap to [0, Len)
	if (Dist < 0.f) Dist += Len; 

	// Grab full transform at distance (world space)
	const FTransform SplineTransform = Spline->GetTransformAtDistanceAlongSpline(Dist, ESplineCoordinateSpace::World);

	Out.Distance = Dist;
	Out.Alpha = (Len > 0.f) ? (Dist / Len) : 0.f;

	// Basis
	const FQuat SplineQuat = SplineTransform.GetRotation();
	const FVector Fwd = SplineQuat.GetForwardVector(); // along track
	const FVector Right = SplineQuat.GetRightVector(); // lateral
	const FVector Up = SplineQuat.GetUpVector();       // vertical

	// Offsets
	FVector SplineLocation = SplineTransform.GetLocation();
	SplineLocation += Right * LateralOffsetCm;
	SplineLocation += Up * VerticalOffsetCm;

	Out.Location = SplineLocation;
	Out.Forward = Fwd.GetSafeNormal();
	Out.Right = Right.GetSafeNormal();
	Out.Up = Up.GetSafeNormal();
	Out.World = FTransform(SplineQuat, SplineLocation, SplineTransform.GetScale3D());
		
	return true;
}

FTransform RogueTrainUtility::SampleTrackFrame(const USplineComponent& Spline, const float Alpha)
{
	const float Len = FMath::Max(1.f, Spline.GetSplineLength());
	const float Dist = WrapTrackAlpha(Alpha) * Len;
	return Spline.GetTransformAtDistanceAlongSpline(Dist, ESplineCoordinateSpace::World);
}

FVector RogueTrainUtility::SampleDockPoint(const USplineComponent& Spline, float Alpha)
{
	const float Len = FMath::Max(1.f, Spline.GetSplineLength());
	const float Dist = WrapTrackAlpha(Alpha) * Len;
	return Spline.GetLocationAtDistanceAlongSpline(Dist, ESplineCoordinateSpace::World);
}

void RogueTrainUtility::BuildPlatformSegment(const USplineComponent& Spline, const FRogueStationConfig& StationConfigData, FRoguePlatformData& Out)
{
	const FTransform StationTransform = SampleTrackFrame(Spline, StationConfigData.TrackAlpha); // track frame
	const FVector Fwd = StationTransform.GetRotation().GetForwardVector();
	const FVector Right = StationTransform.GetRotation().GetRightVector();
	const FVector Up = StationTransform.GetRotation().GetUpVector();
	const FVector Center = StationTransform.GetLocation() + Right * StationConfigData.PlatformConfig.TrackOffset + Up * StationConfigData.PlatformConfig.VerticalOffset;
	const float PlatformHalfLength  = 0.5f * StationConfigData.PlatformConfig.PlatformLength;

	const FQuat Rot = FRotationMatrix::MakeFromXZ(Fwd, Up).ToQuat();
	Out.World = FTransform(Rot, Center, FVector::OneVector);
	Out.Center = Center;
	Out.Fwd = Fwd.GetSafeNormal();
	Out.Right = Right.GetSafeNormal();
	Out.Up = Up.GetSafeNormal();
	Out.Start = Center - Out.Fwd * PlatformHalfLength;
	Out.End = Center + Out.Fwd * PlatformHalfLength;
	Out.PlatformLength = StationConfigData.PlatformConfig.PlatformLength;
	Out.Alpha = StationConfigData.TrackAlpha;
	const float OffsetAlongCm = PlatformHalfLength - 25.f;
	const float TrackLength = Spline.GetSplineLength();
	const float CenterDist = StationConfigData.TrackAlpha * TrackLength;
	float DockDist = FMath::Fmod(CenterDist + OffsetAlongCm, TrackLength);
	if (DockDist < 0.f) DockDist += TrackLength;

	Out.DockAlpha = (TrackLength > 0.f) ? (DockDist / TrackLength) : StationConfigData.TrackAlpha;
	Out.TrackOffset = StationConfigData.PlatformConfig.TrackOffset;
	Out.TrackSide = StationConfigData.PlatformConfig.Side;

	// Bake waiting/spawn points aligned to the straight platform
	Out.WaitingPoints.Reset();
	Out.SpawnPoints.Reset();

	// Set waiting points along platform
	const float Inset = FMath::Clamp(80.f, 0.f, PlatformHalfLength);
	const float Span  = FMath::Max(0.f, StationConfigData.PlatformConfig.PlatformLength - 2.f * Inset);
	const FVector A = Out.Center - Out.Fwd * (0.5f * Span);
	const FVector B = Out.Center + Out.Fwd * (0.5f * Span);
	const int32 WaitNum = FMath::Max(0, StationConfigData.PlatformConfig.WaitingPoints);
	
	for (int32 WaitIdx = 0; WaitIdx < WaitNum; ++WaitIdx)
	{
		const float t = (WaitNum <= 1) ? 0.5f : static_cast<float>(WaitIdx) / static_cast<float>(WaitNum-1);
		Out.WaitingPoints.Add(FMath::Lerp(A, B, t));
	}

	// Set spawn points
	const int32 SpawnNum = FMath::Max(0, StationConfigData.PlatformConfig.SpawnPoints);	
	for (int32 SpawnIdx = 0; SpawnIdx < SpawnNum; ++SpawnIdx)
	{
		FVector Forward = SpawnIdx % 2 == 0 ? Out.Fwd : -Out.Fwd;
		Out.SpawnPoints.Add(Out.Center + Forward  * (StationConfigData.PlatformConfig.SpawnPointDistance));
	}

	Out.WaitingGridConfig = StationConfigData.WaitingGridConfig;
}

void RogueTrainUtility::ComputeConsistPlacement(const FRogueTrackSharedFragment& Track, const float EngineHeadAlpha, const int32 NumCarriages, TArray<FRoguePlacedCar>& Out)
{
	const auto* Settings = GetDefault<URogueDeveloperSettings>();
	if (!Settings) return;
	
	Out.Reset();
	if (!Track.IsValid()) return;

	auto WrappedAlpha = [&](const float Dist){ return RogueTrainUtility::WrapTrackAlpha(Dist / Track.TrackLength); };

	// Convert to spline distance
	const float CarDist = EngineHeadAlpha * Track.TrackLength;

	auto SampleAtDist = [&](const float Dist, const float RideHeight)->FRoguePlacedCar
	{
		RogueTrainUtility::FSplineStationSample Sample;
		const float Alpha = WrappedAlpha(Dist);
		if (!RogueTrainUtility::GetStationSplineSample(Track, Alpha, Sample))
			return { Alpha, FTransform::Identity };

		const FVector Up = Sample.Up.IsNearlyZero() ? FVector::UpVector : Sample.Up;
		const FTransform Transform(FQuat::Identity, Sample.Location + Up * RideHeight, FVector::OneVector);
		return { Alpha, Transform };
	};

	// Engine center = head minus half engine length
	const float EngineCenterDist = CarDist - 0.5f * Settings->EngineLength;
	Out.Add(SampleAtDist(EngineCenterDist, Settings->EngineRideHeight));

	// Walk backwards for carriages 
	float Cursor = EngineCenterDist - 0.5f * Settings->EngineLength - Settings->CarriageSpacing;
	for (int32 i = 0; i < NumCarriages; ++i)
	{
		const float CarCenterDist = Cursor - 0.5f * Settings->CarriageLength;
		Out.Add(SampleAtDist(CarCenterDist, Settings->CarriageRideHeight));

		// move next car to rear face and subtract gap
		Cursor = CarCenterDist - 0.5f * Settings->CarriageLength - Settings->CarriageSpacing;
	}
}

