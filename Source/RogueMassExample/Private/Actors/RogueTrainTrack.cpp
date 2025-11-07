// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/RogueTrainTrack.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Data/RogueDeveloperSettings.h"


// Sets default values
ARogueTrainTrack::ARogueTrainTrack()
{
	PrimaryActorTick.bCanEverTick = false;

	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
	SetRootComponent(SplineComponent);
	SplineComponent->bAllowDiscontinuousSpline = true; 
	SplineComponent->SetClosedLoop(true);
	SplineComponent->SetMobility(EComponentMobility::Static);
}

void ARogueTrainTrack::BuildTrackMeshes()
{
	ClearTrackMeshes();

	const auto* Settings = GetDefault<URogueDeveloperSettings>();
	if (!Settings) return;
	
	if (!TrainTrackMesh || !SplineComponent) return;

	const int32 NumPoints = SplineComponent->GetNumberOfSplinePoints();
	if (NumPoints < 2) return;

	const bool bClosed = SplineComponent->IsClosedLoop();
	const float Length = SplineComponent->GetSplineLength();
	const float SpanLen = FMath::Max(1.f, MeshLength);
	const float StepLen = FMath::Clamp(SpanLen - MeshOverlap, 1.f, SpanLen);
	//const int32 NumSegments = FMath::RoundToInt(Length / MeshLength);
	const int32 NumSegments = FMath::Max(1, static_cast<int32>(FMath::CeilToFloat((bClosed ? Length : (Length - (SpanLen - StepLen))) / StepLen)));


	for (int32 i = 0; i < NumSegments; ++i)
	{
		//const int32 StartIdx = SegIdx;
		//const int32 EndIdx = (SegIdx + 1) % NumPoints;
		//const float StartDist = (SegIdx * Length) / NumSegments;
		//const float EndDistance = ((SegIdx + 1) * Length) / NumSegments;

		float StartDist = i * StepLen;

		// In open splines, stop if a full span would start beyond the end.
		if (!bClosed && StartDist >= Length)
			break;

		float EndDist = StartDist + SpanLen;

		// Wrap/clamp distances
		if (bClosed)
		{
			// keep in [0, SplineLen)
			auto Wrap = [&](float d){ d = FMath::Fmod(d, Length); return (d < 0.f) ? d + Length : d; };
			StartDist = Wrap(StartDist);
			EndDist   = Wrap(EndDist);
		}
		else
		{
			EndDist = FMath::Min(EndDist, Length);
		}

		const FVector StartPos = SplineComponent->GetLocationAtDistanceAlongSpline(StartDist, ESplineCoordinateSpace::Local);
		const FVector EndPos = SplineComponent->GetLocationAtDistanceAlongSpline(EndDist, ESplineCoordinateSpace::Local);
		const FVector StartDir = SplineComponent->GetDirectionAtDistanceAlongSpline(StartDist, ESplineCoordinateSpace::Local);
		const FVector EndDir = SplineComponent->GetDirectionAtDistanceAlongSpline(EndDist, ESplineCoordinateSpace::Local);

		const float ChordLen = (EndPos - StartPos).Size();
		const FVector StartTangent = StartDir * (ChordLen * 0.5f);
		const FVector EndTangent = EndDir * (ChordLen * 0.5f);

		USplineMeshComponent* MeshComponent = NewObject<USplineMeshComponent>(this);
		MeshComponent->SetMobility(EComponentMobility::Static);
		MeshComponent->AttachToComponent(SplineComponent, FAttachmentTransformRules::KeepRelativeTransform);
		MeshComponent->SetRelativeTransform(FTransform::Identity); 
		MeshComponent->SetStaticMesh(TrainTrackMesh);
		MeshComponent->SetForwardAxis(ESplineMeshAxis::X);
		MeshComponent->SetStartAndEnd(StartPos, StartTangent, EndPos, EndTangent);
		MeshComponent->SetStartScale(FVector2D(TrainTrackScale, TrainTrackScale));
		MeshComponent->SetEndScale  (FVector2D(TrainTrackScale, TrainTrackScale));
		MeshComponent->SetStartRoll(0.f);
		MeshComponent->SetEndRoll(0.f);		
		MeshComponent->bSmoothInterpRollScale = true;

		if (TrackMaterial)
		{
			MeshComponent->SetMaterial(0, TrackMaterial);
		}

		MeshComponent->RegisterComponent();
		TrackSegments.Add(MeshComponent);
        AddInstanceComponent(MeshComponent);
	}
}

void ARogueTrainTrack::ClearTrackMeshes()
{
	for (USplineMeshComponent* TrackMesh : TrackSegments)
	{
		if (!TrackMesh) continue;
		TrackMesh->DestroyComponent();		
	}
	
	TrackSegments.Reset();
}

