// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/RogueTrainWorldSubsystem.h"
#include "Data/RogueDeveloperSettings.h"
#include "EngineUtils.h"
#include "MassCommonFragments.h"
#include "MassEntityConfigAsset.h"
#include "MassEntitySubsystem.h"
#include "MassSpawnerSubsystem.h"
#include "Actors/RogueTrainStation.h"
#include "GameFramework/Actor.h"
#include "Components/SplineComponent.h"
#include "Utilities/RogueTrainUtility.h"


void URogueTrainWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	InitEntityManagement();
}

void URogueTrainWorldSubsystem::Deinitialize()
{	
	PendingSpawns.Reset();
	EntityPool.Empty();
	WorldEntities.Empty();
	Stations.Reset();
	TrackSpline = nullptr;
	EntityManager = nullptr;

	StopSpawnManager();

	Super::Deinitialize();
}

void URogueTrainWorldSubsystem::StartSpawnManager()
{
	const UWorld* World = GetWorld();
	if (!World) return;
	
	const auto* Settings = GetDefault<URogueDeveloperSettings>();
	if (!Settings) return;
	
	if (!World->GetTimerManager().IsTimerActive(SpawnTimerHandle))
	{
		World->GetTimerManager().SetTimer(SpawnTimerHandle, this, &ThisClass::SpawnManager, 0.1f, true);
	}
}

void URogueTrainWorldSubsystem::SpawnManager()
{
	ProcessPendingSpawns();

	/*UE_LOG(LogTemp, Warning, TEXT("[Stations:%d][Engines:%d][Carriages:%d][Passengers:%d] PendingSpawns:%d"),
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
	
	if (USplineComponent* Found = TrackActor->FindComponentByClass<USplineComponent>())
	{
		TrackSpline = Found;
	}
}

void URogueTrainWorldSubsystem::GatherStationActors()
{
	Stations.Reset();
	if (!GetWorld()) return;

	for (TActorIterator<ARogueTrainStation> It(GetWorld()); It; ++It)
	{
		const ARogueTrainStation* Station = *It;
		if (!Station) continue;

		FRogueStationData StationData;
		StationData.Alpha = Station->GetStationT();
		StationData.WorldPos = Station->GetActorLocation();
		StationData.WaitingPoints = Station->WaitingPoints;
		StationData.SpawnPoints   = Station->SpawnPoints;
		Stations.Add(StationData);
	}

	Stations.Sort([](const FRogueStationData& L, const FRogueStationData& R){ return L.Alpha < R.Alpha; });
}

void URogueTrainWorldSubsystem::CreateStations()
{
	// Check station actors found
	checkf(Stations.Num() > 0, TEXT("No stations found in world! Place ARogueTrainStation actors to define stations."));
	
	// One-time station spawn (authoring actors already placed)
	const UWorld* World = GetWorld();
	if (!World) return;

	const auto* Settings = GetDefault<URogueDeveloperSettings>();
	if (!Settings) return;

	// Setup station entity configuration template
	if (Settings->StationConfig.IsNull()) return;
	const UMassEntityConfigAsset* StationCfg = Settings->StationConfig.LoadSynchronous();
	if (!StationCfg)
	{
		UE_LOG(LogTemp, Error, TEXT("StationConfig is not set or failed to load."));
		return;
	}
	const FMassEntityTemplate& StationTemplate = StationCfg->GetOrCreateEntityTemplate(*World);
	if (!StationTemplate.IsValid()) return;

	// Create station entities at station actor locations
	for (int i = 0; i < Stations.Num(); ++i)
	{
		FRogueSpawnRequest Request;
		Request.Type = ERogueEntityType::Station;
		Request.EntityTemplate = StationTemplate;
		Request.RemainingCount = 1;
		Request.SpawnLocation = Stations[i].WorldPos;
		Request.StationData = Stations[i];
		Request.StationIndex = i;

		EnqueueSpawns(Request);				
	}
}

void URogueTrainWorldSubsystem::CreateTrains()
{
	const UWorld* World = GetWorld();
	if (!World) return;

	const auto* Settings = GetDefault<URogueDeveloperSettings>();
	if (!Settings) return;

	// Setup train entity configuration templates
	if (Settings->TrainEngineConfig.IsNull() || Settings->TrainCarriageConfig.IsNull()) return;
	const UMassEntityConfigAsset* TrainEngineCfg = Settings->TrainEngineConfig.LoadSynchronous();
	checkf(TrainEngineCfg != nullptr, TEXT("TrainEngineConfig is not set or failed to load."));
	const UMassEntityConfigAsset* TrainCarriageCfg = Settings->TrainCarriageConfig.LoadSynchronous();
	checkf(TrainCarriageCfg != nullptr, TEXT("TrainCarriageConfig is not set or failed to load."));
	const FMassEntityTemplate& TrainEngineTemplate = TrainEngineCfg->GetOrCreateEntityTemplate(*World);	
	const FMassEntityTemplate& TrainCarriageTemplate = TrainCarriageCfg->GetOrCreateEntityTemplate(*World);
	if (!TrainEngineTemplate.IsValid() || !TrainCarriageTemplate.IsValid()) return;

	const FRogueTrackSharedFragment& TrackSharedFragment = GetTrackShared();
	if (!TrackSharedFragment.IsValid()) return;

	const int32 NumStations = TrackSharedFragment.StationTrackAlphas.Num();
	if (NumStations <= 0) return;
	
	const int32 NumberOfTrains = Settings->NumTrains;	
	const int32 CarriagesPer = Settings->CarriagesPerTrain;
	const float SpacingMeters = Settings->CarriageSpacingMeters;
	const int32 CapacityPerCar = Settings->MaxPassengersPerCarriage;
	const int32 Passes = FMath::DivideAndRoundUp(NumberOfTrains, NumStations);
	
	for (int i = 0; i < NumberOfTrains; ++i)
	{
		const int32 StationIdx = i % NumStations;
		const int32 PassIdx = i / NumStations;
		const int32 NextIdx = (StationIdx + 1) % NumStations;
		const float T0 = TrackSharedFragment.StationTrackAlphas[StationIdx];
		const float T1 = TrackSharedFragment.StationTrackAlphas[NextIdx];
		const float dT = RogueTrainUtility::ArcDistanceWrapped(T0, T1);  // segment length in normalized
		const float Frac = (Passes <= 1) ? 0.f : static_cast<float>(PassIdx) / static_cast<float>(Passes); // 0 (at station), 0.5 (mid), 0.333 etc.
		const float TrainAlpha = RogueTrainUtility::WrapTrackAlpha(T0 + dT * Frac);

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
		Request.SpawnLocation = Sample.Location;
		Request.StartAlpha = TrainAlpha; 
		Request.StationIndex = StationIdx;

		TWeakObjectPtr<URogueTrainWorldSubsystem> TrainSubsystemWeak = this;
		Request.OnSpawned = [TrainAlpha, CarriagesPer, SpacingMeters, CapacityPerCar, TrainCarriageTemplate, TrainSubsystemWeak](const TArray<FMassEntityHandle>& Spawned)
		{
			auto* TrainSubsystemLocal = TrainSubsystemWeak.Get();
			if (Spawned.Num() == 0 || !TrainSubsystemLocal) return;
			const FMassEntityHandle LeadHandle = Spawned[0];

			const FRogueTrackSharedFragment& TrackSharedFragment = TrainSubsystemLocal->GetTrackShared();
			if (!TrackSharedFragment.IsValid()) return;

			for (int32 c = 1; c <= CarriagesPer; ++c)
			{
				// Position behind the engine and other carriages
				const float backDistCm = (c * SpacingMeters) * 100.f;
				const float dT = backDistCm / TrackSharedFragment.TrackLength;
				const float CarriageAlpha = RogueTrainUtility::WrapTrackAlpha(TrainAlpha - dT);

				RogueTrainUtility::FSplineStationSample CarriageSample;
				if (!RogueTrainUtility::GetStationSplineSample(TrackSharedFragment, CarriageAlpha, CarriageSample))
					continue;
				
				FRogueSpawnRequest CarReq;
				CarReq.Type				= ERogueEntityType::TrainCarriage;
				CarReq.EntityTemplate	= TrainCarriageTemplate;
				CarReq.RemainingCount	= 1;
				CarReq.LeadHandle		= LeadHandle;
				CarReq.CarriageIndex	= c;
				CarReq.SpacingMeters	= SpacingMeters;
				CarReq.CarriageCapacity	= CapacityPerCar;
				CarReq.StartAlpha		= CarriageAlpha;                 // sets FRogueSplineFollowFragment.Alpha on spawn
				CarReq.SpawnLocation	= CarriageSample.Location;   // optional: if your template has Transform

				TrainSubsystemLocal->EnqueueSpawns(CarReq);
			}
		};

		EnqueueSpawns(Request);
	}
}

void URogueTrainWorldSubsystem::BuildTrackShared()
{
	CachedTrack = FRogueTrackSharedFragment{}; // reset
	CachedTrack.Spline = TrackSpline.Get();
	CachedTrack.StationTrackAlphas.Reset();
	CachedTrack.StationWorldPositions.Reset();
	const USplineComponent* Spline = TrackSpline.Get();
	CachedTrack.TrackLength = Spline ? Spline->GetSplineLength() : 0.f;
	
	struct FStationTmp { float Alpha; FVector WorldPos; };
	TArray<FStationTmp> Tmp;
	Tmp.Reserve(Stations.Num());

	for (const auto& StationData : Stations)
	{
		FStationTmp Entry;
		Entry.Alpha = RogueTrainUtility::WrapTrackAlpha(StationData.Alpha);
		Entry.WorldPos = StationData.WorldPos;
		Tmp.Add(Entry);
	}

	// Sort ascending by normalized T
	Tmp.Sort([](const FStationTmp& A, const FStationTmp& B){ return A.Alpha < B.Alpha; });
	
	for (int32 i = 0; i < Tmp.Num(); ++i)
	{
		CachedTrack.StationTrackAlphas.Add(Tmp[i].Alpha);
		CachedTrack.StationWorldPositions.Add(Tmp[i].WorldPos);
	}

	bTrackDirty = false;
	++TrackRevision;
}

const FRogueTrackSharedFragment& URogueTrainWorldSubsystem::GetTrackShared()
{
	if (bTrackDirty) BuildTrackShared();
	return CachedTrack;
}

void URogueTrainWorldSubsystem::EnqueueSpawns(const FRogueSpawnRequest& Request)
{
	if (!Request.EntityTemplate.IsValid() || Request.RemainingCount <= 0) return;
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
			Spawner->SpawnEntities(Request.EntityTemplate, Need, Spawned);
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

void URogueTrainWorldSubsystem::EnqueueEntityToPool(const FMassEntityHandle Entity, const ERogueEntityType Type)
{
	if (!EntityManager || !Entity.IsValid()) return;

	UnregisterEntity(Type, Entity);

	// disable LOD (optional)
	/*if (auto* LOD = EntityManager->GetFragmentDataPtr<FMassRepresentationLODFragment>(Entity))
	{
		LOD->LOD = EMassLOD::Off;
	}*/
	
	// sink it
	if (auto* TransformFragment = EntityManager->GetFragmentDataPtr<FTransformFragment>(Entity))
	{
		TransformFragment->GetMutableTransform().SetLocation(FVector(0,0,-100000.f));
	}

	// strip per-kind runtime tags you don’t want on pooled (add/remove what you use)
	/*EntityManager->Defer().PushCommand<FMassCommandRemoveTag<FRoguePassengerWaitingTag>>(Entity);
	EntityManager->Defer().PushCommand<FMassCommandRemoveTag<FRogueTrainPassengerTag>>(Entity);
	EntityManager->Defer().PushCommand<FMassCommandRemoveTag<FRogueTrainEngineTag>>(Entity);
	EntityManager->Defer().PushCommand<FMassCommandRemoveTag<FRogueTrainCarriageTag>>(Entity);
	EntityManager->Defer().PushCommand<FMassCommandRemoveTag<FRogueTrainStationTag>>(Entity);*/

	// mark pooled
	EntityManager->Defer().PushCommand<FMassCommandAddTag<FRoguePooledEntityTag>>(Entity);

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
		RegisterEntity(Type, EntityHandle);
	}
	
	return Available;
}

void URogueTrainWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	StartSpawnManager();	
	GatherStationActors();
	CreateStations();	
	DiscoverSplineFromSettings();
	CreateTrains();
}

void URogueTrainWorldSubsystem::ConfigureSpawnedEntity(const FRogueSpawnRequest& Request, const FMassEntityHandle Entity) const
{
	// Position
	if (FTransformFragment* TransformFragment = EntityManager->GetFragmentDataPtr<FTransformFragment>(Entity))
	{
		TransformFragment->GetMutableTransform().SetLocation(Request.SpawnLocation);
	}

	switch (Request.Type)
	{
		case ERogueEntityType::Station:
		{
			if (auto* Ref = EntityManager->GetFragmentDataPtr<FRogueStationRefFragment>(Entity))
			{
				Ref->StationIndex = Request.StationIndex;
			}
			break;
		}
		case ERogueEntityType::TrainEngine:
		{
			if (auto* State = EntityManager->GetFragmentDataPtr<FRogueTrainStateFragment>(Entity))
			{
				State->bAtStation = true;
				State->TargetStationIndex = Request.StationIndex;
				State->PreviousStationIndex = Request.StationIndex;
				State->DwellTimeRemaining = 2.f;
			}
				
			if (auto* Follow = EntityManager->GetFragmentDataPtr<FRogueSplineFollowFragment>(Entity))
			{
				Follow->Alpha = Request.StartAlpha;
				Follow->Speed = 0.f;
			}
			else
			{
				// Move entity to an archetype that contains this fragment and initialize it
				FRogueSplineFollowFragment InitFollow;
				InitFollow.Alpha = Request.StartAlpha;
				InitFollow.Speed = 0.f;

				EntityManager->Defer().PushCommand<FMassCommandAddFragmentInstances>(Entity, InitFollow);
			}
			break;
		}
		case ERogueEntityType::TrainCarriage:
		{
			if (auto* Link = EntityManager->GetFragmentDataPtr<FRogueTrainLinkFragment>(Entity))
			{
				Link->LeadHandle = Request.LeadHandle;
				Link->CarriageIndex= Request.CarriageIndex;
				Link->SpacingMeters= Request.SpacingMeters;
			}
			if (auto* Cap = EntityManager->GetFragmentDataPtr<FRogueCarriageCapacityFragment>(Entity))
			{
				Cap->Capacity = Request.CarriageCapacity;
				Cap->Occupied = 0;
			}
			if (auto* Follow = EntityManager->GetFragmentDataPtr<FRogueSplineFollowFragment>(Entity))
			{
				Follow->Alpha = Request.StartAlpha;
				Follow->Speed = 0.f;
			}
			break;
		}
		case ERogueEntityType::Passenger:
		{
			if (auto* Route = EntityManager->GetFragmentDataPtr<FRoguePassengerRouteFragment>(Entity))
			{
				Route->OriginStationIndex = Request.OriginStationIndex;
				Route->DestStationIndex = Request.DestStationIndex;
				Route->WaitingPointIdx = Request.WaitingPointIdx;
			}
				
			if (auto* SRef = EntityManager->GetFragmentDataPtr<FRogueStationRefFragment>(Entity))
			{
				SRef->StationIndex = Request.OriginStationIndex;
			}
			break;
		}
		default: break;
	}
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
