// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/RogueTrainStation.h"
#include "Data/RogueDeveloperSettings.h"
#include "Components/SplineComponent.h"

ARogueTrainStation::ARogueTrainStation()
{
	PrimaryActorTick.bCanEverTick = false;
}

#if WITH_EDITOR
void ARogueTrainStation::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ComputeStationT();
}
#endif

void ARogueTrainStation::ComputeStationT()
{
	if (!GetWorld()) return;
	const auto* Settings = GetDefault<URogueDeveloperSettings>();
	if (!Settings || !Settings->TrackSplineActor.IsValid()) return;
	const AActor* TrackActor = Settings->TrackSplineActor.Get();
	if (!TrackActor) return;
	const USplineComponent* Spline = TrackActor->FindComponentByClass<USplineComponent>();
	if (!Spline) return;
	
	// Project to distance & normalize
	const FVector Platform = GetActorLocation();
	const float RealDist = Spline->GetDistanceAlongSplineAtLocation(Platform, ESplineCoordinateSpace::World);
	const float Len = FMath::Max(1.f, Spline->GetSplineLength());
	StationT = RealDist / Len;
}