// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/RogueTrainWorldSubsystem.h"
#include "Data/RogueDeveloperSettings.h"
#include "EngineUtils.h"
#include "MassCommonFragments.h"
#include "MassEntityConfigAsset.h"
#include "MassEntitySubsystem.h"
#include "MassRepresentationFragments.h"
#include "MassSpawnerSubsystem.h"
#include "Actors/RogueTrainStation.h"
#include "Actors/RogueTrainTrack.h"
#include "Avoidance/MassAvoidanceFragments.h"
#include "GameFramework/Actor.h"
#include "Components/SplineComponent.h"
#include "Utilities/RoguePassengerUtility.h"
#include "Utilities/RogueStationQueueUtility.h"
#include "Utilities/RogueTrainUtility.h"


void URogueTrainWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	InitEntityManagement();
	InitTemplateConfigs();
	bTrackDirty = true;

#if WITH_EDITOR
	InitDebugData();
#endif
	
}

void URogueTrainWorldSubsystem::Deinitialize()
{	
	PendingSpawns.Reset();
	EntityPool.Empty();
	WorldEntities.Empty();
	StationActorData.Reset();
	TrackSpline = nullptr;
	EntityManager = nullptr;

	StopSpawnManager();

	Super::Deinitialize();
}

void URogueTrainWorldSubsystem::InitTemplateConfigs()
{
	const auto* Settings = GetDefault<URogueDeveloperSettings>();
	if (!Settings) return;
	
	if (!Settings->StationConfig.IsNull())
	{
		StationConfig = Settings->StationConfig.LoadSynchronous();
	}
	
	if (!Settings->TrainEngineConfig.IsNull())
	{
		TrainConfig = Settings->TrainEngineConfig.LoadSynchronous(); 
	}
	
	if (!Settings->TrainCarriageConfig.IsNull())
	{
		CarriageConfig = Settings->TrainCarriageConfig.LoadSynchronous(); 
	}
	
	if (!Settings->PassengerConfig.IsNull())
	{
		PassengerConfig = Settings->PassengerConfig.LoadSynchronous(); 
	}
}

void URogueTrainWorldSubsystem::InitConfigTemplates(const UWorld& InWorld)
{
	if (StationConfig)
	{
		StationTemplate = StationConfig->GetOrCreateEntityTemplate(InWorld);
	}
	
	if (TrainConfig)
	{
		TrainTemplate = TrainConfig->GetOrCreateEntityTemplate(InWorld);
	}
	
	if (CarriageConfig)
	{
		CarriageTemplate = CarriageConfig->GetOrCreateEntityTemplate(InWorld);
	}
	
	if (PassengerConfig)
	{
		PassengerTemplate = PassengerConfig->GetOrCreateEntityTemplate(InWorld);
	}
}

void URogueTrainWorldSubsystem::StartSpawnManager()
{
	const UWorld* World = GetWorld();
	if (!World) return;
	
	if (!World->GetTimerManager().IsTimerActive(SpawnTimerHandle))
	{
		World->GetTimerManager().SetTimer(SpawnTimerHandle, this, &ThisClass::SpawnManager, 0.1f, true);
	}
}

void URogueTrainWorldSubsystem::SpawnManager()
{
	ProcessPendingSpawns();

	/*UE_LOG(LogTemp, Warning, TEXT("[StationActorData:%d][Engines:%d][Carriages:%d][Passengers:%d] PendingSpawns:%d"),
		GetLiveCount(ERogueEntityType::Station),
		GetLiveCount(ERogueEntityType::TrainEngine),
		GetLiveCount(ERogueEntityType::TrainCarriage),
		GetLiveCount(ERogueEntityType::Passenger),
		PendingSpawns.Num()
	);*/
}

void URogueTrainWorldSubsystem::StopSpawnManager()
{
	const UWorld* World = GetWorld();
	if (!World) return;
	
	World->GetTimerManager().ClearTimer(SpawnTimerHandle);
}

void URogueTrainWorldSubsystem::InitEntityManagement()
{
	if (auto* EntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>())
	{
		EntityManager = &EntitySubsystem->GetMutableEntityManager();
	}
	
	check(EntityManager);

	for (ERogueEntityType K : {
		ERogueEntityType::Station,
		ERogueEntityType::TrainEngine,
		ERogueEntityType::TrainCarriage,
		ERogueEntityType::Passenger
	})
	{
		EntityPool.FindOrAdd(K);
		WorldEntities.FindOrAdd(K);
	}
}

void URogueTrainWorldSubsystem::DiscoverSplineFromSettings()
{
	const auto* Settings = GetDefault<URogueDeveloperSettings>();
	if (!Settings || !Settings->TrackSplineActor.IsValid()) return;

	AActor* TrackActor = Settings->TrackSplineActor.Get();
	if (!TrackActor) return;
	
	TrackActors.Add(Cast<ARogueTrainTrack>(TrackActor));
	if (USplineComponent* Found = TrackActor->FindComponentByClass<USplineComponent>())
	{
		TrackSpline = Found;

		if (TrackSpline.Get())
		{
			ResampleSplineUniform(*TrackSpline.Get(), Settings->TrackSplineResampleStep);
		}
	}
}

void URogueTrainWorldSubsystem::GatherStationActors()
{
	StationActorData.Reset();
	if (!GetWorld()) return;

	for (TActorIterator<ARogueTrainStation> It(GetWorld()); It; ++It)
	{
		const ARogueTrainStation* Station = *It;
		if (!Station) continue;

		FRogueStationData StationData;
		StationData.Alpha = Station->GetStationAlpha();
		StationData.WorldPos = Station->GetActorLocation();

		StationData.WaitingPoints.Reset();
		StationData.SpawnPoints.Reset();

		const FTransform ToWorld = Station->GetActorTransform();
		for (const FTransform& LT : Station->WaitingPointWidgets)
		{
			StationData.WaitingPoints.Add(ToWorld.TransformPosition(LT.GetLocation()));
		}
		
		for (const FTransform& LT : Station->SpawnPointWidgets)
		{
			StationData.SpawnPoints.Add(ToWorld.TransformPosition(LT.GetLocation()));
		}
		
		StationActorData.Add(StationData);
	}

	StationActorData.Sort([](const FRogueStationData& L, const FRogueStationData& R){ return L.Alpha < R.Alpha; });
}

void URogueTrainWorldSubsystem::CreateStations()
{
	// Build platform data from settings
	BuildStationPlatformData();
	
	// Check station data found
	checkf(Platforms.Num() > 0, TEXT("No stations found! Configure station data in Settings."));
		
	const FMassEntityTemplate* StationEntityTemplate = GetStationTemplate();
	if (!StationEntityTemplate->IsValid()) return;

	// Create station entities at platform locations	
	for (int i = 0; i < Platforms.Num(); ++i)
	{
		FRogueSpawnRequest Request;
		Request.Type = ERogueEntityType::Station;
		Request.EntityTemplate = StationEntityTemplate;
		Request.RemainingCount = 1;
		Request.PlatformData = Platforms[i];
		Request.StationIdx = i;
		Request.Transform = Platforms[i].World;

		EnqueueSpawns(Request);				
	}
}

void URogueTrainWorldSubsystem::ConfigureTrackToStation(const FRogueSpawnRequest& Request, const float ResampleDistance) const
{
	USplineComponent* Spline = TrackSpline.Get();
	if (!Spline) return;

	const FVector Center = Request.PlatformData.Center;
	const float PlatformLength = FMath::Max(1.f, Request.PlatformData.PlatformLength);
	const float PlatformHalfLength = PlatformLength * 0.5f;
	const float SampleDistance = ResampleDistance + PlatformLength;
	const float TrackOffset = Request.PlatformData.TrackOffset;
	const float SplineLength = Spline->GetSplineLength();
	const FVector Fwd = Request.PlatformData.Fwd;
	const FVector Up = Request.PlatformData.Up;	
	const FVector Right = FVector::CrossProduct(Up, Fwd).GetSafeNormal();
	const int32 NumPoints = Spline->GetNumberOfSplinePoints();	
	float DistCenter = Spline->GetDistanceAlongSplineAtLocation(Center, ESplineCoordinateSpace::World);
	float DistStart = DistCenter - 0.5f * SampleDistance;
	float DistEnd = DistCenter + 0.5f * SampleDistance;
	const bool bWrap = (DistEnd < DistStart);

	// Choose offset side
	float Sign = +1.f;
	EPlatformSide TrackSide = Request.PlatformData.TrackSide;
	if (TrackSide == EPlatformSide::Left)  Sign = -1.f;
	if (TrackSide == EPlatformSide::Auto)
	{
		// Pick whichever offset line is closer to the given center
		const FVector Check1 = Center + Right * (+TrackOffset);
		const FVector Check2 = Center + Right * (-TrackOffset);
		Sign = (FVector::DistSquared(Check1, Center) <= FVector::DistSquared(Check2, Center)) ? +1.f : -1.f;
	}
	const FVector PlatformSplinePos = Center + Fwd + Right * (Sign * TrackOffset);
	const FVector PlatformStartPos = Center - Fwd * PlatformHalfLength + Right * (Sign * TrackOffset);
	const FVector PlatformEndPos = Center + Fwd * PlatformHalfLength + Right * (Sign * TrackOffset);
	const FVector PlatformDirection = (PlatformEndPos - PlatformStartPos).GetSafeNormal();

	// Collect point indices inside [DistStart, DistEnd]
	TArray<int32> Window;
	for (int32 i = 0; i < NumPoints; ++i)
	{
		const float PointDistance = Spline->GetDistanceAlongSplineAtSplinePoint(i);

		// Check if point is within platform distance window
		if ((!bWrap && PointDistance >= DistStart && PointDistance <= DistEnd) || ( bWrap && (PointDistance >= DistStart || PointDistance <= DistEnd)))
		{
			Window.Add(i);
		}
	}
	if (Window.Num() == 0) return;
	
	// For stable edge snapping, find exact indices nearest to start/end distances
	auto NearestIndexToDistance = [&](const float TargetPoint)->int32
	{
		int32 Best = Window[0];
		float BestEndDist = FLT_MAX;
		for (const int32 Point : Window)
		{
			float PointDistance = Spline->GetDistanceAlongSplineAtSplinePoint(Point);
			if (bWrap && PointDistance < DistStart)
			{
				PointDistance += SplineLength;
			}
			
			const float EndDist = FMath::Abs(PointDistance - (bWrap && TargetPoint < DistStart ? TargetPoint + SplineLength : TargetPoint));
			if (EndDist < BestEndDist)
			{
				BestEndDist = EndDist;
				Best = Point;
			}
		}
		
		return Best;
	};
	
	const int32 PlatformStartIndex = NearestIndexToDistance(DistStart);
	const int32 PlatformEndIndex = NearestIndexToDistance(DistEnd);

	// Unwrap window indices if needed
	const float Span = (bWrap ? (DistEnd + SplineLength - DistStart) : (DistEnd - DistStart));
	auto DistToT = [&](float InDistance)->float
	{
		if (bWrap && InDistance < DistStart)
		{
			InDistance += SplineLength;
		}
		
		return FMath::Clamp((InDistance - DistStart) / FMath::Max(1.f, Span), 0.f, 1.f);
	};

	auto PrevIdx = [&](const int32 Idx)
	{
		return (Idx-1 >= 0) ? Idx-1 : (Spline->IsClosedLoop() ? NumPoints-1 : 0);
	};
	
	auto NextIdx = [&](const int32 Idx)
	{
		return (Idx+1 <  NumPoints) ? Idx+1 : (Spline->IsClosedLoop() ? 0 : NumPoints-1);
	};

	// Apply linear alignment inside window
	const FRotator PlatformRotation = FRotationMatrix::MakeFromXZ(PlatformDirection, Up).Rotator();
	for (const int32 PointIndex : Window)
	{
		const float PointDistance = Spline->GetDistanceAlongSplineAtSplinePoint(PointIndex);
		const float PointAlpha = DistToT(PointDistance);
		const FVector PointPosition = FMath::Lerp(PlatformStartPos, PlatformEndPos, PointAlpha);

		Spline->SetLocationAtSplinePoint(PointIndex, PointPosition, ESplineCoordinateSpace::World, false);
		Spline->SetRotationAtSplinePoint(PointIndex, PlatformRotation, ESplineCoordinateSpace::World, false);
		Spline->SetTangentAtSplinePoint(PointIndex, FVector::ZeroVector, ESplineCoordinateSpace::World, false);
		Spline->SetSplinePointType(PointIndex, ESplinePointType::Linear, false);
	}

	// Snap edges exactly
	Spline->SetLocationAtSplinePoint(PlatformStartIndex, PlatformStartPos, ESplineCoordinateSpace::World, false);
	Spline->SetLocationAtSplinePoint(PlatformEndIndex, PlatformEndPos, ESplineCoordinateSpace::World, false);

	// Set departing and approach tangents
	const int32 PrevEndIndex = PrevIdx(PlatformEndIndex);
	const int32 NextEndIndex = NextIdx(PlatformEndIndex);
	const FVector EndPrevPosition = Spline->GetLocationAtSplinePoint(PrevEndIndex, ESplineCoordinateSpace::World);
	const FVector EndNextPosition = Spline->GetLocationAtSplinePoint(NextEndIndex, ESplineCoordinateSpace::World);
	const FVector EndDirection = (PlatformEndPos - EndPrevPosition).GetSafeNormal();
	const float EndLength = (PlatformEndPos - EndNextPosition).Size();
	const float EndMagnitude = EndLength * 0.5f;
	const FVector EndTangent = EndDirection * EndMagnitude;
	Spline->SetTangentAtSplinePoint(PlatformEndIndex, EndTangent, ESplineCoordinateSpace::World, false);

	const int32 PrevStartIndex = PrevIdx(PlatformStartIndex);
	const int32 NextStartIndex = NextIdx(PlatformStartIndex);
	const FVector StartPrevPosition = Spline->GetLocationAtSplinePoint(PrevStartIndex, ESplineCoordinateSpace::World);
	const FVector StartNextPosition = Spline->GetLocationAtSplinePoint(NextStartIndex, ESplineCoordinateSpace::World);
	const FVector StartDirection = (StartNextPosition - PlatformStartPos).GetSafeNormal();
	const float StartLength = (PlatformEndPos - StartPrevPosition).Size();
	const float StartMagnitude = StartLength * 0.5f;
	const FVector StartTangent = StartDirection * StartMagnitude;
	Spline->SetTangentAtSplinePoint(PlatformStartIndex, StartTangent, ESplineCoordinateSpace::World, false);	
	
	Spline->UpdateSpline();	
}

void URogueTrainWorldSubsystem::GetStationSide(const FRoguePlatformData& PlatformData, const FTransform& StationTransform, float& Out)
{
	const FVector Fwd = PlatformData.Fwd;
	const FVector Up = PlatformData.Up;	
	const FVector Right = FVector::CrossProduct(Up, Fwd).GetSafeNormal();
	
	if (PlatformData.TrackSide == EPlatformSide::Left)
	{
		Out = -1.f;
	}
	else if (PlatformData.TrackSide == EPlatformSide::Auto)
	{
		// pick the side whose offset line is closer to the provided platform center
		const FVector RightCandidate = StationTransform.GetLocation() + Right *  PlatformData.TrackOffset;
		const FVector LeftCandidate = StationTransform.GetLocation() - Right *  PlatformData.TrackOffset;
		Out = (FVector::DistSquared(RightCandidate, PlatformData.Center) < FVector::DistSquared(LeftCandidate, PlatformData.Center)) ? +1.f : -1.f;
	}
}

void URogueTrainWorldSubsystem::BuildStationPlatformData()
{
	const auto* Settings = GetDefault<URogueDeveloperSettings>();
	if (!Settings || !Settings->TrackSplineActor.IsValid()) return;

	const TWeakObjectPtr<USplineComponent> Spline = TrackSpline.Get();
	if (!Spline.IsValid()) return;

	// Copy and sort by alpha so next station is defined correctly
	TArray<FRogueStationConfig> Stations = Settings->Stations;
	Algo::SortBy(Stations, &FRogueStationConfig::TrackAlpha);
	
	Platforms.Reset();

	// Create platform data for each station in alpha order
	TArray<float> StationTrackAlphas;
	for (int i = 0; i < Stations.Num(); ++i)
	{
		const FRogueStationConfig& StationConfigData = Stations[i];
		StationTrackAlphas.Add(RogueTrainUtility::WrapTrackAlpha(StationConfigData.TrackAlpha));
		
		FRoguePlatformData PlatformSegment;
		RogueTrainUtility::BuildPlatformSegment(*Spline, StationConfigData, PlatformSegment);
		Platforms.Add(MoveTemp(PlatformSegment));
	}
}

void URogueTrainWorldSubsystem::CreateTrains()
{
	const auto* Settings = GetDefault<URogueDeveloperSettings>();
	if (!Settings) return;

	const FRogueTrackSharedFragment& TrackSharedFragment = GetTrackShared();
	if (!TrackSharedFragment.IsValid()) return;

	// Setup train entity configuration templates
	const FMassEntityTemplate* TrainEngineTemplate = GetTrainTemplate();	
	const FMassEntityTemplate* TrainCarriageTemplate = GetCarriageTemplate();
	if (!TrainEngineTemplate->IsValid() || !TrainCarriageTemplate->IsValid()) return;

	const int32 NumStations = TrackSharedFragment.StationEntities.Num();
	if (NumStations <= 0) return;
	
	const int32 NumberOfTrains = Settings->NumTrains;	
	const int32 CarriagesPer = Settings->CarriagesPerTrain;
	const int32 CapacityPerCar = Settings->MaxPassengersPerCarriage;
	const int32 Passes = FMath::DivideAndRoundUp(NumberOfTrains, NumStations);
	
	for (int i = 0; i < NumberOfTrains; ++i)
	{
		const int32 StationIdx = i % NumStations;
		const int32 PassIdx = i / NumStations;
		const int32 NextIdx = (StationIdx + 1) % NumStations;
		const float T0 = TrackSharedFragment.GetStationAlphaByIndex(StationIdx);
		const float T1 = TrackSharedFragment.GetStationAlphaByIndex(NextIdx);
		const float dT = RogueTrainUtility::ArcDistanceWrapped(T0, T1);
		const float Frac = (Passes <= 1) ? 0.f : static_cast<float>(PassIdx) / static_cast<float>(Passes);
		const float TrainAlpha = RogueTrainUtility::WrapTrackAlpha(T0 + dT * Frac);

		// Compute full consist placement from this head alpha
		TArray<FRoguePlacedCar> Placement;
		RogueTrainUtility::ComputeConsistPlacement(TrackSharedFragment, TrainAlpha, CarriagesPer, Placement);
		if (Placement.Num() == 0) continue;

		RogueTrainUtility::FSplineStationSample Sample;
		if (!RogueTrainUtility::GetStationSplineSample(TrackSharedFragment, TrainAlpha, Sample))
		{
			/*Along*/ //0.f,        // e.g. +100.f to place a bit ahead
			/*Lateral*/ //0.f,      // e.g. +150.f to offset to platform side
			/*Vertical*/ //0.f,     // e.g. +5.f lift
			continue;
		}		
			
		FRogueSpawnRequest Request;
		Request.Type = ERogueEntityType::TrainEngine;
		Request.EntityTemplate = TrainEngineTemplate;
		Request.RemainingCount = 1;
		Request.Transform = Placement[0].Transform;               // with ride height
		Request.StartAlpha = Placement[0].Alpha;
		Request.StationIdx = StationIdx;

		TWeakObjectPtr<URogueTrainWorldSubsystem> TrainSubsystemWeak = this;
		Request.OnSpawned = [Placement, CapacityPerCar, TrainCarriageTemplate, TrainSubsystemWeak, Settings](const TArray<FMassEntityHandle>& Spawned)
		{
			auto* TrainSubsystemLocal = TrainSubsystemWeak.Get();
			if (Spawned.Num() == 0 || !TrainSubsystemLocal) return;
			const FMassEntityHandle LeadHandle = Spawned[0];

			const FRogueTrackSharedFragment& TrackSharedFragment = TrainSubsystemLocal->GetTrackShared();
			if (!TrackSharedFragment.IsValid()) return;
			
			const float DerivedSpacing = (Settings->CarriageLength + Settings->CarriageSpacing);

			for (int32 c = 1; c < Placement.Num(); ++c)
			{				
				FRogueSpawnRequest CarriageRequest;
				CarriageRequest.Type = ERogueEntityType::TrainCarriage;
				CarriageRequest.EntityTemplate = TrainCarriageTemplate;
				CarriageRequest.RemainingCount = 1;
				CarriageRequest.LeadHandle = LeadHandle;
				CarriageRequest.CarriageIndex = c;
				CarriageRequest.Spacing = DerivedSpacing;
				CarriageRequest.CarriageCapacity = CapacityPerCar;
				CarriageRequest.StartAlpha = Placement[c].Alpha;
				CarriageRequest.Transform = Placement[c].Transform;

				TrainSubsystemLocal->EnqueueSpawns(CarriageRequest);
			}
		};

		EnqueueSpawns(Request);
	}
}

void URogueTrainWorldSubsystem::BuildTrackSharedData()
{
	const auto* Settings = GetDefault<URogueDeveloperSettings>();
	if (!Settings) return;
	
	CachedTrack = FRogueTrackSharedFragment{};
	CachedTrack.Spline = TrackSpline.Get();
	if (!CachedTrack.Spline.IsValid()) return;

	const USplineComponent& Spline = *CachedTrack.Spline.Get();
	CachedTrack.TrackLength = Spline.GetSplineLength();

	if (Platforms.Num() == 0) return;
	CachedTrack.StationEntities.Reset(Platforms.Num());
	CachedTrack.Platforms.Reset(Platforms.Num());
	
	for (int i = 0; i < Platforms.Num(); ++i)
	{
		// Find the station by alpha to ensure alpha ordering matches entity ordering
		if (const FMassEntityHandle* StationEntity = StationEntities.Find(i))
		{
			CachedTrack.StationEntities.Emplace(i, *StationEntity);
		}
		
		CachedTrack.Platforms.Add(Platforms[i]);
	}

	bTrackDirty = false;
	++TrackRevision;
}

const FRogueTrackSharedFragment& URogueTrainWorldSubsystem::GetTrackShared()
{
	if (bTrackDirty) BuildTrackSharedData();
	return CachedTrack;
}

void URogueTrainWorldSubsystem::EnqueueSpawns(const FRogueSpawnRequest& Request)
{
	if (!Request.EntityTemplate->IsValid() || Request.RemainingCount <= 0) return;
	PendingSpawns.Add(Request);
}

void URogueTrainWorldSubsystem::ProcessPendingSpawns()
{
	if (!EntityManager || PendingSpawns.Num() == 0) return;
	
	const auto* Settings = GetDefault<URogueDeveloperSettings>();
	if (!Settings) return;

	int32 Budget = Settings->MaxSpawnsPerFrame;	

	auto* Spawner = GetWorld()->GetSubsystem<UMassSpawnerSubsystem>();
	auto* MassEntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
	if (!Spawner || !MassEntitySubsystem) return;

	// reverse so we can RemoveAtSwap
	for (int32 i = PendingSpawns.Num()-1; i >= 0 && Budget > 0; --i)
	{
		FRogueSpawnRequest& Request = PendingSpawns[i];
		const int32 ThisBatch = FMath::Min(Request.RemainingCount, Budget);

		TArray<FMassEntityHandle> NewEntities;
		const int32 Reused = RetrievePooledEntities(Request.Type, ThisBatch, NewEntities);

		if (Reused < ThisBatch)
		{
			const int32 Need = ThisBatch - Reused;
			TArray<FMassEntityHandle> Spawned;
			Spawner->SpawnEntities(*Request.EntityTemplate, Need, Spawned);
			NewEntities.Append(Spawned);
		}

		// Configure fragments/tags/position here (per entity)
		FMassEntityManager& EntityManagerMutable = MassEntitySubsystem->GetMutableEntityManager();
		for (const FMassEntityHandle NewEntity : NewEntities)
		{
			RegisterEntity(Request.Type, NewEntity);
			ConfigureSpawnedEntity(Request, NewEntity);

			// clear pool marker if present
			EntityManagerMutable.Defer().PushCommand<FMassCommandRemoveTag<FRoguePooledEntityTag>>(NewEntity);
		}
		
		if (Request.OnSpawned)
		{
			// Pass back completed handles this interval
			Request.OnSpawned(NewEntities);   
		}

		Request.RemainingCount -= ThisBatch;
		Budget -= ThisBatch;

		if (Request.RemainingCount <= 0)
		{
			PendingSpawns.RemoveAtSwap(i);
		}
	}
}

void URogueTrainWorldSubsystem::ResampleSplineUniform(USplineComponent& Spline, float Step)
{
	if (Step <= 1.f) Step = 1.f;

	const bool bClosed = Spline.IsClosedLoop();
	const float Len = Spline.GetSplineLength();

	// Sample transforms along the *original* spline
	TArray<FVector> PointsLocal;
	TArray<FVector> TangentLocal;
	TArray<FRotator> RotationLocal;

	const int32 NumSegments = FMath::Max(1, FMath::FloorToInt(Len / Step));
	const int32 NumPts = bClosed ? NumSegments : (NumSegments + 1);

	PointsLocal.Reserve(NumPts);
	TangentLocal.Reserve(NumPts);
	RotationLocal.Reserve(NumPts);

	// Helper to convert World→Local for stability
	const FTransform CompXf = Spline.GetComponentTransform();
	const FTransform InvCompXf = CompXf.Inverse();

	for (int32 i = 0; i < NumPts; ++i)
	{
		const float Dist = FMath::Min(i * Step, Len);
		const FTransform XfW = Spline.GetTransformAtDistanceAlongSpline(Dist, ESplineCoordinateSpace::World);
		const FVector DirW = Spline.GetDirectionAtDistanceAlongSpline(Dist, ESplineCoordinateSpace::World);

		PointsLocal.Add(InvCompXf.TransformPosition(XfW.GetLocation()));
		RotationLocal.Add((InvCompXf.GetRotation() * XfW.GetRotation()).Rotator());

		// Tangent magnitude - half the step keeps smooth curvature
		TangentLocal.Add(InvCompXf.TransformVector(DirW * (Step * 0.5f)));
	}

	// Rebuild points
	Spline.ClearSplinePoints(false);
	for (int32 i = 0; i < NumPts; ++i)
	{
		Spline.AddSplinePoint(PointsLocal[i], ESplineCoordinateSpace::Local, false);
		Spline.SetSplinePointType(i, ESplinePointType::Curve, false);
		Spline.SetTangentAtSplinePoint(i, TangentLocal[i], ESplineCoordinateSpace::Local, false);
		Spline.SetRotationAtSplinePoint(i, RotationLocal[i], ESplineCoordinateSpace::Local, false);
	}

	Spline.SetClosedLoop(bClosed, false);
	Spline.UpdateSpline();
}

void URogueTrainWorldSubsystem::EnqueueEntityToPool(const FMassEntityHandle Entity, const FMassExecutionContext& Context, const ERogueEntityType Type)
{
	if (!EntityManager || !Entity.IsValid()) return;

	UnregisterEntity(Type, Entity);

	if (Type == ERogueEntityType::Passenger)
	{
		RoguePassengerUtility::HidePassenger(*EntityManager, Entity);
	}
	
	// mark pooled
	Context.Defer().PushCommand<FMassCommandAddTag<FRoguePooledEntityTag>>(Entity);

	EntityPool.FindOrAdd(Type).Add(Entity);
}

int32 URogueTrainWorldSubsystem::RetrievePooledEntities(const ERogueEntityType Type, const int32 Count, TArray<FMassEntityHandle>& Out)
{
	TArray<FMassEntityHandle>& Pool = GetEntitiesFromPoolByType(Type);
	const int32 Available = FMath::Min(Count, Pool.Num());
	for (int32 i = 0; i < Available; ++i)
	{
		FMassEntityHandle EntityHandle = Pool.Pop(EAllowShrinking::No);
		if (!EntityHandle.IsValid()) continue;
		
		Out.Add(EntityHandle);
	}
	
	return Available;
}

const FMassEntityTemplate* URogueTrainWorldSubsystem::GetStationTemplate() const
{
	return StationTemplate.IsValid() ? &StationTemplate : nullptr;
}

const FMassEntityTemplate* URogueTrainWorldSubsystem::GetTrainTemplate() const
{
	return TrainTemplate.IsValid() ? &TrainTemplate	: nullptr;
}

const FMassEntityTemplate* URogueTrainWorldSubsystem::GetCarriageTemplate() const
{
	return CarriageTemplate.IsValid() ? &CarriageTemplate : nullptr;
}

const FMassEntityTemplate* URogueTrainWorldSubsystem::GetPassengerTemplate() const
{
	return PassengerTemplate.IsValid() ? &PassengerTemplate : nullptr;
}

void URogueTrainWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	
	DiscoverSplineFromSettings();
	InitConfigTemplates(InWorld);
	StartSpawnManager();	
	CreateStations();	
}

void URogueTrainWorldSubsystem::ConfigureSpawnedEntity(const FRogueSpawnRequest& Request, const FMassEntityHandle Entity) 
{
	if (!EntityManager) return;
	
	// Position
	if (FTransformFragment* TransformFragment = EntityManager->GetFragmentDataPtr<FTransformFragment>(Entity))
	{
		TransformFragment->GetMutableTransform().SetLocation(Request.Transform.GetLocation());
		TransformFragment->GetMutableTransform().SetRotation(Request.Transform.GetRotation());
	}

	// Type-specific configuration
	switch (Request.Type)
	{
		case ERogueEntityType::Station:
		{
			ConfigureStation(Request, Entity);				
			break;
		}
		case ERogueEntityType::TrainEngine:
		{
			ConfigureTrain(Request, Entity);				
			break;
		}
		case ERogueEntityType::TrainCarriage:
		{
			ConfigureCarriage(Request, Entity);
			break;
		}
		case ERogueEntityType::Passenger:
		{
			ConfigurePassenger(Request, Entity);
			break;
		}
		default: break;
	}
}

void URogueTrainWorldSubsystem::ConfigureStation(const FRogueSpawnRequest& Request, const FMassEntityHandle Entity)
{
	const auto* Settings = GetDefault<URogueDeveloperSettings>();
	if (!Settings) return;

	if (!EntityManager) return;

	// Add station entity with alpha key
	StationEntities.Add(Request.StationIdx, Entity);

	// Mark track dirty to rebuild cached data
	bTrackDirty = true;
				
	if (auto* StationFragment = EntityManager->GetFragmentDataPtr<FRogueStationFragment>(Entity))
	{
		StationFragment->StationIndex = Request.StationIdx;
		StationFragment->WorldPosition = Request.PlatformData.Center;				
	}
				
	if (auto* QueueFragment = EntityManager->GetFragmentDataPtr<FRogueStationQueueFragment>(Entity))
	{
		QueueFragment->SpawnPoints = Request.PlatformData.SpawnPoints;
		QueueFragment->WaitingPoints = Request.PlatformData.WaitingPoints;
		QueueFragment->WaitingGridConfig = Request.PlatformData.WaitingGridConfig;

		QueueFragment->Grids.Reset();
		for (int WaitIdx = 0; WaitIdx < QueueFragment->WaitingPoints.Num(); ++WaitIdx)
		{
			const FVector WaitingPoint = QueueFragment->WaitingPoints[WaitIdx];
			RogueStationQueueUtility::BuildGridForWaitingPoint(Request.PlatformData, *QueueFragment, WaitingPoint, WaitIdx);					
		}
	}

	ConfigureTrackToStation(Request, Settings->TrackSplineResampleStep);

	const int32 Slot = GetStationDebugIndex();
	if (auto* DebugSlotFragment = EntityManager->GetFragmentDataPtr<FRogueDebugSlotFragment>(Entity))
	{
		if (DebugSlotFragment->Slot == INDEX_NONE)
		{
			DebugSlotFragment->Slot = Slot;
		}				
	}			
				
	// Once all stations are created, create trains and track meshes
	if (StationEntities.Num() == Settings->Stations.Num()) // All stations created
	{
		// Create track meshes
		for (int i = 0; i < TrackActors.Num(); ++i)
		{
			TrackActors[i]->BuildTrackMeshes();
		}
				
		CreateTrains();
	}
	
#if WITH_EDITOR
	// Debug
	DrawDebugStations(GetWorld());
#endif
}

void URogueTrainWorldSubsystem::ConfigureTrain(const FRogueSpawnRequest& Request, const FMassEntityHandle Entity)
{
	const auto* Settings = GetDefault<URogueDeveloperSettings>();
	if (!Settings) return;

	if (!EntityManager) return;

	if (auto* State = EntityManager->GetFragmentDataPtr<FRogueTrainStateFragment>(Entity))
	{
		State->bAtStation = true;
		State->TargetStationIdx = Request.StationIdx;
		State->PreviousStationIndex = Request.StationIdx;
		State->StationTimeRemaining = 2.f;
	}
				
	if (auto* Follow = EntityManager->GetFragmentDataPtr<FRogueTrainTrackFollowFragment>(Entity))
	{
		Follow->Alpha = Request.StartAlpha;
		Follow->Speed = 0.f;
	}
	else
	{
		// Move entity to an archetype that contains this fragment and initialize it
		FRogueTrainTrackFollowFragment InitFollow;
		InitFollow.Alpha = Request.StartAlpha;
		InitFollow.Speed = 0.f;

		EntityManager->Defer().PushCommand<FMassCommandAddFragmentInstances>(Entity, InitFollow);
	}

	const int32 Slot = GetTrainDebugIndex();
	if (auto* DebugSlotFragment = EntityManager->GetFragmentDataPtr<FRogueDebugSlotFragment>(Entity))
	{
		if (DebugSlotFragment->Slot == INDEX_NONE)
		{
			DebugSlotFragment->Slot = Slot;
		}				
	}

	CarriageCounts.Add(Entity, 0);
}

void URogueTrainWorldSubsystem::ConfigureCarriage(const FRogueSpawnRequest& Request, const FMassEntityHandle Entity)
{
	const auto* Settings = GetDefault<URogueDeveloperSettings>();
	if (!Settings) return;

	if (!EntityManager) return;

	if (auto* Link = EntityManager->GetFragmentDataPtr<FRogueTrainLinkFragment>(Entity))
	{
		Link->LeadHandle = Request.LeadHandle;
		Link->CarriageIndex= Request.CarriageIndex;
		Link->Spacing= Request.Spacing;
	}
				
	if (auto* CarriageFragment = EntityManager->GetFragmentDataPtr<FRogueCarriageFragment>(Entity))
	{
		CarriageFragment->Capacity = Request.CarriageCapacity;
		CarriageFragment->Occupants.Reserve(Request.CarriageCapacity);
		CarriageFragment->NextAllowedUnloadTime = GetWorld()->GetTimeSeconds() + FMath::FRandRange(0.f, Settings->UnloadStartJitter);
		CarriageFragment->UnloadCursor = 0;
	}
				
	if (auto* Follow = EntityManager->GetFragmentDataPtr<FRogueTrainTrackFollowFragment>(Entity))
	{
		Follow->Alpha = Request.StartAlpha;
		Follow->Speed = 0.f;
	}

	const int32 Slot = GetCarriageDebugIndex();
	if (auto* DebugSlotFragment = EntityManager->GetFragmentDataPtr<FRogueDebugSlotFragment>(Entity))
	{
		if (DebugSlotFragment->Slot == INDEX_NONE)
		{
			DebugSlotFragment->Slot = Slot;
		}				
	}

	CarriageCounts.FindOrAdd(Request.LeadHandle)++;
	LeadToCarriages.FindOrAdd(Request.LeadHandle).Add(Entity);
}

void URogueTrainWorldSubsystem::ConfigurePassenger(const FRogueSpawnRequest& Request, const FMassEntityHandle Entity)
{
	const auto* Settings = GetDefault<URogueDeveloperSettings>();
	if (!Settings) return;

	if (!EntityManager) return;

	if (auto* PassengerFragment = EntityManager->GetFragmentDataPtr<FRoguePassengerFragment>(Entity))
	{
		PassengerFragment->OriginStation = Request.OriginStation;
		PassengerFragment->DestinationStation = Request.DestinationStation;
		PassengerFragment->VehicleHandle = FMassEntityHandle();
		PassengerFragment->MaxSpeed = Request.MaxSpeed;
		PassengerFragment->Target = Request.Transform.GetLocation();
		PassengerFragment->WaitingPointIdx = INDEX_NONE;
		PassengerFragment->WaitingSlotIdx = INDEX_NONE;
		PassengerFragment->bWaiting = false;
		PassengerFragment->Phase = ERoguePassengerPhase::EnteredWorld;
	}
	if (auto* RadiusFragment = EntityManager->GetFragmentDataPtr<FAgentRadiusFragment>(Entity))
	{
		RadiusFragment->Radius = Settings->PassengerRadius; 
	}				

	const int32 Slot = GetPassengerDebugSlot();
	if (auto* DebugSlotFragment = EntityManager->GetFragmentDataPtr<FRogueDebugSlotFragment>(Entity))
	{
		if (DebugSlotFragment->Slot == INDEX_NONE)
		{
			DebugSlotFragment->Slot = Slot;
		}				
	}

	RoguePassengerUtility::ShowPassenger(*EntityManager, Entity, Request.Transform.GetLocation());
}

void URogueTrainWorldSubsystem::RegisterEntity(const ERogueEntityType Type, const FMassEntityHandle Entity)
{
	GetEntitiesFromWorldByType(Type).Add(Entity);
}

void URogueTrainWorldSubsystem::UnregisterEntity(const ERogueEntityType Type, const FMassEntityHandle Entity)
{
	if (auto* Arr = WorldEntities.Find(Type))
	{
		Arr->RemoveSwap(Entity);
	}
}

int32 URogueTrainWorldSubsystem::GetTotalLiveCount() const
{
	int32 Sum = 0;
	for (const auto& KV : WorldEntities) Sum += KV.Value.Num();
	return Sum;
}

int32 URogueTrainWorldSubsystem::GetTotalPoolCount() const
{
	int32 Sum = 0;
	for (const auto& KV : EntityPool) Sum += KV.Value.Num();
	return Sum;
}


#if WITH_EDITOR
// Rebuild track when settings change
void URogueTrainWorldSubsystem::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	if (Event.Property && Event.Property->GetOwnerClass() == URogueDeveloperSettings::StaticClass())
	{
		bTrackDirty = true;
	}
}

// Debug

void URogueTrainWorldSubsystem::InitDebugData()
{
	const auto* Settings = GetDefault<URogueDeveloperSettings>();
	if (!Settings) return;

	PassengersDebugSnapshot.Init(FRogueDebugPassenger(), Settings->MaxPassengersOverall);
	TrainsDebugSnapshot.Init(FRogueDebugTrain(), Settings->NumTrains);
	CarriagesDebugSnapshot.Init(FRogueDebugCarriage(), Settings->NumTrains * Settings->CarriagesPerTrain);
	StationsDebugSnapshot.Init(FRogueDebugStation(), Settings->Stations.Num());
}

void URogueTrainWorldSubsystem::DrawDebugStations(const UWorld* InWorld)
{
	const auto* Settings = GetDefault<URogueDeveloperSettings>();
	if (!Settings) return;

	for (const auto& It : StationEntities)
	{
		if (auto* QueueFragment = EntityManager->GetFragmentDataPtr<FRogueStationQueueFragment>(It.Value))
		{
			if (Settings->bDrawStationSpawnPoints)
			{
				for (const FVector& SpawnPosition : QueueFragment->SpawnPoints)
				{
					DrawDebugSphere(InWorld, SpawnPosition, 20.f, 8, FColor::Red, true, 30.f);
				}				
			}
					
			if (Settings->bDrawStationWaitPoints)
			{				
				for (const FVector& WaitPosition : QueueFragment->WaitingPoints)
				{
					DrawDebugSphere(GetWorld(), WaitPosition, 20.f, 8, FColor::Blue, true, 30.f);
				}				
			}

			if (Settings->bDrawStationWaitGrid)
			{
				for (auto Pair : QueueFragment->Grids)
				{
					const FRogueWaitingGrid& GridData = Pair.Value;
					for (FVector GridPoint : GridData.SlotPositions)
					{
						GridPoint.Z += 20.f;
						DrawDebugSphere(GetWorld(), GridPoint, 5.f, 8, FColor::Black, true, 30.f);
					}
				}
			}
		}
	}
}

#endif
