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
	
	if (USplineComponent* Found = TrackActor->FindComponentByClass<USplineComponent>())
	{
		TrackSpline = Found;
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
		Request.Transform = FTransform(Sample.Location);
		Request.StartAlpha = TrainAlpha; 
		Request.StationIdx = StationIdx;

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
				
				FRogueSpawnRequest CarriageRequest;
				CarriageRequest.Type = ERogueEntityType::TrainCarriage;
				CarriageRequest.EntityTemplate = TrainCarriageTemplate;
				CarriageRequest.RemainingCount = 1;
				CarriageRequest.LeadHandle = LeadHandle;
				CarriageRequest.CarriageIndex = c;
				CarriageRequest.SpacingMeters = SpacingMeters;
				CarriageRequest.CarriageCapacity = CapacityPerCar;
				CarriageRequest.StartAlpha = CarriageAlpha;
				CarriageRequest.Transform = FTransform(CarriageSample.Location);

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
	CachedTrack.StationTrackAlphas.Reset(Platforms.Num());
	CachedTrack.Platforms.Reset(Platforms.Num());
	
	for (int i = 0; i < Platforms.Num(); ++i)
	{
		// Find the station by alpha to ensure alpha ordering matches entity ordering
		if (const FMassEntityHandle* StationEntity = StationEntities.Find(i))
		{
			CachedTrack.StationEntities.Emplace(i, *StationEntity);
		}
		
		CachedTrack.StationTrackAlphas.Add(Platforms[i].Alpha);
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
	const auto* Settings = GetDefault<URogueDeveloperSettings>();
	if (!Settings) return;

	if (!EntityManager) return;
	
	// Position
	if (FTransformFragment* TransformFragment = EntityManager->GetFragmentDataPtr<FTransformFragment>(Entity))
	{
		TransformFragment->GetMutableTransform().SetLocation(Request.Transform.GetLocation());
		TransformFragment->GetMutableTransform().SetRotation(Request.Transform.GetRotation());
	}

	switch (Request.Type)
	{
		case ERogueEntityType::Station:
		{
			// Add station entity with alpha key
			StationEntities.Add(Request.StationIdx, Entity);

			// Mark track dirty to rebuild cached data
			bTrackDirty = true;
				
			if (auto* StationFragment = EntityManager->GetFragmentDataPtr<FRogueStationFragment>(Entity))
			{
				StationFragment->StationIndex = Request.StationIdx;
				StationFragment->StationAlpha = Request.PlatformData.Alpha;
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

			const int32 Slot = GetStationDebugIndex();
			if (auto* DebugSlotFragment = EntityManager->GetFragmentDataPtr<FRogueDebugSlotFragment>(Entity))
			{
				if (DebugSlotFragment->Slot == INDEX_NONE)
				{
					DebugSlotFragment->Slot = Slot;
				}				
			}
				
			// Once all stations are created, create trains
			if (StationEntities.Num() == Settings->Stations.Num()) // All stations created
			{					
				CreateTrains();
			}
#if WITH_EDITOR
				// Debug
				DrawDebugStations(GetWorld());
#endif
				
			break;
		}
		case ERogueEntityType::TrainEngine:
		{
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
			break;
		}
		case ERogueEntityType::Passenger:
		{
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
				PassengerFragment->Phase = ERoguePassengerPhase::ToStationWaitingPoint;
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
	
	for (const TPair<float, FMassEntityHandle>& It : StationEntities)
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
