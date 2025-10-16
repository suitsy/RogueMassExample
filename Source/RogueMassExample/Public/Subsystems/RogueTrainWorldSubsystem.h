// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityTemplate.h"
#include "Mass/Fragments/RogueFragments.h"
#include "Subsystems/WorldSubsystem.h"
#include "RogueTrainWorldSubsystem.generated.h"

class USplineComponent;

UENUM()
enum class ERogueEntityType : uint8
{
	Station,
	TrainEngine,
	TrainCarriage,
	Passenger
};

USTRUCT()
struct ROGUEMASSEXAMPLE_API FRogueStationData
{
	GENERATED_BODY()
	UPROPERTY() float Alpha = 0.f;
	UPROPERTY() FVector WorldPos = FVector::ZeroVector;
	UPROPERTY() TArray<FVector> WaitingPoints;
	UPROPERTY() TArray<FVector> SpawnPoints;
};

USTRUCT()
struct ROGUEMASSEXAMPLE_API FRogueSpawnRequest
{
	GENERATED_BODY()

	UPROPERTY() ERogueEntityType Type = ERogueEntityType::Passenger;
	FMassEntityTemplate EntityTemplate = FMassEntityTemplate();
	UPROPERTY() int32 RemainingCount = 0;

	// Any
	UPROPERTY() FVector SpawnLocation = FVector::ZeroVector;
	UPROPERTY() float StartAlpha = 0.f; 

	// Station
	UPROPERTY() int32 StationIndex = INDEX_NONE;
	UPROPERTY() FRogueStationData StationData;

	// Carriage
	UPROPERTY() FMassEntityHandle LeadHandle; 
	UPROPERTY() int32 CarriageIndex = 1;      
	UPROPERTY() float SpacingMeters = 8.f;                  
	UPROPERTY() int32 CarriageCapacity = 20;                 

	// Passenger
	UPROPERTY() int32 OriginStationIndex = INDEX_NONE;       
	UPROPERTY() int32 DestStationIndex = INDEX_NONE;		 
	UPROPERTY() int32 WaitingPointIdx = INDEX_NONE;

	// Completion callback
	TFunction<void(const TArray<FMassEntityHandle>& /*Spawned*/)> OnSpawned = nullptr;
};

/**
 * 
 */
UCLASS()
class ROGUEMASSEXAMPLE_API URogueTrainWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	USplineComponent* GetSpline() const { return TrackSpline.Get(); }
	const TArray<FRogueStationData>& GetStations() const { return Stations; }

	// Build shared track fragment
	void BuildTrackShared();
	void InvalidateTrackShared() { bTrackDirty = true; }
	const FRogueTrackSharedFragment& GetTrackShared();
	int32 GetTrackRevision() const { return TrackRevision; }
	
	// Queue a spawn using the template you created from Dev Settings
	void EnqueueSpawns(const FRogueSpawnRequest& Request);

	// Drive this per-frame (e.g., from a simple “driver” processor or your GameMode tick)
	void ProcessPendingSpawns();

	// Pooling (generic)
	void EnqueueEntityToPool(const FMassEntityHandle Entity, const ERogueEntityType Type);
	int32 RetrievePooledEntities(const ERogueEntityType Type, const int32 Count, TArray<FMassEntityHandle>& Out);

protected:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
	UPROPERTY() TWeakObjectPtr<USplineComponent> TrackSpline;
	UPROPERTY() TArray<FRogueStationData> Stations;
	UPROPERTY() TArray<FRogueSpawnRequest> PendingSpawns;
	UPROPERTY() FRogueTrackSharedFragment CachedTrack;   // cached once, reused everywhere
	UPROPERTY() int32 TrackRevision = 0;
	UPROPERTY() bool bTrackDirty = true;
	TMap<ERogueEntityType, TArray<FMassEntityHandle>> EntityPool;
	TMap<ERogueEntityType, TArray<FMassEntityHandle>> WorldEntities;

	void StartSpawnManager();
	void SpawnManager();
	void StopSpawnManager();
	void InitEntityManagement();
	void DiscoverSplineFromSettings();
	void GatherStationActors();
	void CreateStations();
	void CreateTrains();

	// Cache
	FMassEntityManager* EntityManager = nullptr;

	// Helpers
	void ConfigureSpawnedEntity(const FRogueSpawnRequest& Request, const FMassEntityHandle Entity) const;
	void RegisterEntity(const ERogueEntityType Type, const FMassEntityHandle Entity);
	void UnregisterEntity(const ERogueEntityType Type, const FMassEntityHandle Entity);
	
	TArray<FMassEntityHandle>& GetEntitiesFromPoolByType(const ERogueEntityType Type) {	return EntityPool.FindOrAdd(Type); }
	TArray<FMassEntityHandle>& GetEntitiesFromWorldByType(const ERogueEntityType Type) { return WorldEntities.FindOrAdd(Type); }

	FTimerHandle SpawnTimerHandle;

public:
	// Read-only accessors
	const TArray<FMassEntityHandle>& GetLiveEntities(const ERogueEntityType Type) const { return WorldEntities.FindChecked(Type); }
	int32 GetLiveCount(const ERogueEntityType Type) const { if (const auto* A = WorldEntities.Find(Type)) return A->Num(); return 0; }
	int32 GetPoolCount(const ERogueEntityType Type) const { if (const auto* A = EntityPool.Find(Type)) return A->Num(); return 0; }
	int32 GetTotalLiveCount() const;
	int32 GetTotalPoolCount() const;
};
