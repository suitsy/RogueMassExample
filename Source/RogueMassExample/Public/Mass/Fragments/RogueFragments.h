// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityHandle.h"
#include "MassEntityTypes.h"
#include "RogueFragments.generated.h"

class USplineComponent;

USTRUCT() struct ROGUEMASSEXAMPLE_API FRogueTrainEngineTag : public FMassTag { GENERATED_BODY() };
USTRUCT() struct ROGUEMASSEXAMPLE_API FRogueTrainCarriageTag : public FMassTag { GENERATED_BODY() };
USTRUCT() struct ROGUEMASSEXAMPLE_API FRogueTrainStationTag : public FMassTag { GENERATED_BODY() };
USTRUCT() struct ROGUEMASSEXAMPLE_API FRogueTrainPassengerTag : public FMassTag { GENERATED_BODY() };

USTRUCT() struct ROGUEMASSEXAMPLE_API FRogueTrainAtStationTag : public FMassTag { GENERATED_BODY() };
USTRUCT() struct ROGUEMASSEXAMPLE_API FRogueTrainDoorsOpenTag : public FMassTag { GENERATED_BODY() };
USTRUCT() struct ROGUEMASSEXAMPLE_API FRogueTrainReadyDepartTag : public FMassTag { GENERATED_BODY() };

USTRUCT() struct ROGUEMASSEXAMPLE_API FRoguePassengerWaitingTag : public FMassTag { GENERATED_BODY() };
USTRUCT() struct ROGUEMASSEXAMPLE_API FRoguePassengerOnTrainTag : public FMassTag { GENERATED_BODY() };
USTRUCT() struct ROGUEMASSEXAMPLE_API FRoguePassengerDespawnTag : public FMassTag { GENERATED_BODY() };

USTRUCT() struct ROGUEMASSEXAMPLE_API FRoguePooledEntityTag : public FMassTag { GENERATED_BODY() };

UENUM()
enum class ERoguePassengerPhase : uint8
{
	EnteredWorld,						// 0) just spawned
	ToStationWaitingPoint,				// 1) spawn -> nearest station waiting point
	ToAssignedCarriage,					// 2) waiting -> carriage
	RideOnTrain,						// 
	UnloadAtStation,					// 
	ToPostUnloadWaitingPoint,			// 4) after unload -> nearest waiting point (brief settle)
	ToExitSpawn,						// 5) waiting -> nearest station spawn
	Pool								// 6) ready to destroy/return to pool
};

UENUM()
enum class ERogueStationTrainPhase : uint8
{
	NotStopped,
	Arriving,
	Unloading,
	Loading,
	Departing,
};

UENUM()
enum class EPlatformSide : uint8
{
	Right,
	Left,
	Auto
};

USTRUCT(BlueprintType)
struct FRoguePlatformConfig
{
	GENERATED_BODY()

	// Platform extent along its forward axis (cm).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0"))
	float PlatformLength = 1000.f;

	// Local offsets relative to the track at this station
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float TrackOffset = 150.f;   // +Right from track to platform edge

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float VerticalOffset = 0.f;    // lift platform up/down

	// How far from the platform center will the spawn points be
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float SpawnPointDistance = 750.f;

	// Waiting point density for passengers
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0"))
	int32 WaitingPoints = 10;
	
	// Spawn points for passengers
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0"))
	int32 SpawnPoints = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EPlatformSide Side = EPlatformSide::Auto;
};

USTRUCT(BlueprintType)
struct FRogueStationWaitingGridConfig
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0"))
	int32 GridCols = 4;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0"))
	int32 GridRows = 2;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0"))
	float GridColSpacing = 55.f;  // along platform
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0"))
	float GridRowSpacing = 45.f;  // away from platform edge (toward interior)
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0"))
	float GridEdgeInset  = 80.f;  // away from platform edge (toward interior)
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0"))
	float GridOffset  = 80.f;
};

USTRUCT(BlueprintType)
struct FRogueStationConfig
{
	GENERATED_BODY()

	// Normalized parameter on the track [0..1]
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.0", ClampMax="1.0"))
	float TrackAlpha = 0.f;

	// Per-station visual/geometry tuning
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FRoguePlatformConfig PlatformConfig;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FRogueStationWaitingGridConfig WaitingGridConfig;
};

USTRUCT()
struct FRogueWaitingGrid
{
	GENERATED_BODY()

	/** World-space slots centers, created once when stations are built */
	TArray<FVector> SlotPositions;

	/** Who is in each slot, or invalid if free */
	TArray<FMassEntityHandle> OccupiedBy;

	FORCEINLINE bool IsValidSlotIndex(const int32 Idx) const { return SlotPositions.IsValidIndex(Idx); }
};

USTRUCT()
struct ROGUEMASSEXAMPLE_API FRoguePassengerQueueEntry
{
	GENERATED_BODY()

	FMassEntityHandle Passenger = FMassEntityHandle();
	FMassEntityHandle DestStation = FMassEntityHandle();
	int32 WaitingPointIdx = INDEX_NONE;
	float EnqueuedGameTime = 0.f;
	int32 Priority = 0;
};

USTRUCT()
struct FStationRef
{
	GENERATED_BODY()
	
	float Alpha = 0.f; 
	FMassEntityHandle Entity;
};

USTRUCT()
struct ROGUEMASSEXAMPLE_API FRogueStationQueueFragment : public FMassFragment
{
	GENERATED_BODY()
	
	TMap<int32, TArray<FRoguePassengerQueueEntry>> QueuesByWaitingPoint;
	TMap<int32, FRogueWaitingGrid> Grids;
	TArray<FVector> WaitingPoints;
	TArray<FVector> SpawnPoints;	
	FRogueStationWaitingGridConfig WaitingGridConfig;
};

USTRUCT()
struct FRoguePlatformData
{
	GENERATED_BODY()
	
	// Straight segment representing the platform line in world space
	FVector Start = FVector::ZeroVector;
	FVector End = FVector::ZeroVector;

	// Convenience frame at the platform center
	FVector Center = FVector::ZeroVector;
	FVector Fwd = FVector::ForwardVector;
	FVector Right = FVector::RightVector;
	FVector Up = FVector::UpVector;
	float DockAlpha = 0.f;
	float TrackOffset = 0.f;
	float PlatformLength = 1000.f;
	EPlatformSide TrackSide = EPlatformSide::Auto;
	
	FTransform World = FTransform::Identity;
	float Alpha = 0.f;   // normalized [0..1]
	TArray<FVector> WaitingPoints;
	TArray<FVector> SpawnPoints;
	FRogueStationWaitingGridConfig WaitingGridConfig;
};

USTRUCT()
struct ROGUEMASSEXAMPLE_API FRogueTrainTrackFollowFragment : public FMassFragment
{
	GENERATED_BODY()
	
	float Alpha = 0.f; // [0..1] along spline
	float Speed = 0.f;  // cm/s
	FVector WorldPos = FVector::ZeroVector;
	FVector WorldFwd = FVector::ForwardVector;
};

USTRUCT()
struct ROGUEMASSEXAMPLE_API FRogueStationFragment : public FMassFragment
{
	GENERATED_BODY()
	
	int32 StationIndex = INDEX_NONE;
	FVector WorldPosition = FVector::ZeroVector;
	FVector WorldTrackPoint = FVector::ZeroVector;	
};

USTRUCT()
struct ROGUEMASSEXAMPLE_API FRogueTrainStateFragment : public FMassFragment
{
	GENERATED_BODY()
	
	bool bIsStopping = false;
	bool bAtStation = false;
	bool bHasArrived = false;
	ERogueStationTrainPhase StationTrainPhase = ERogueStationTrainPhase::NotStopped;
	float HeadwaySpeedScale = 1.f;
	float StationTimeRemaining = 0.f;  
	float PrevAlpha = 0.f;  
	FMassEntityHandle TargetStation = FMassEntityHandle();
	int32 TargetStationIdx = INDEX_NONE;
	int32 PreviousStationIndex = INDEX_NONE;
};

USTRUCT()
struct ROGUEMASSEXAMPLE_API FRogueTrainLinkFragment : public FMassFragment
{
	GENERATED_BODY()
	
	FMassEntityHandle LeadHandle;
	int32 CarriageIndex = 0; // 0 reserved for lead
	float Spacing = 8.f;
};

USTRUCT()
struct ROGUEMASSEXAMPLE_API FRogueCarriageFragment : public FMassFragment
{
	GENERATED_BODY()
	
	int32 Capacity = 100;
	TArray<FMassEntityHandle> Occupants;
	float NextAllowedUnloadTime = 0.f;
	int32 UnloadCursor = 0; 
};

USTRUCT()
struct ROGUEMASSEXAMPLE_API FRoguePassengerFragment : public FMassFragment
{
	GENERATED_BODY()
	
	FMassEntityHandle OriginStation = FMassEntityHandle();       
	FMassEntityHandle DestinationStation = FMassEntityHandle();
	int32 WaitingPointIdx = INDEX_NONE;
	int32 WaitingSlotIdx = INDEX_NONE;
	int32 CarriageIndex = INDEX_NONE;
	FMassEntityHandle VehicleHandle;
	ERoguePassengerPhase Phase = ERoguePassengerPhase::ToStationWaitingPoint;
	FVector Target = FVector::ZeroVector;
	float AcceptanceRadius = 20.f;
	float MaxSpeed = 200.f;
	bool bWaiting = false;
};

USTRUCT()
struct ROGUEMASSEXAMPLE_API FRoguePassengerHandleFragment : public FMassFragment
{
	GENERATED_BODY()
	
	FMassEntityHandle RidingTrain;
};

USTRUCT()
struct FRogueDebugSlotFragment : public FMassFragment
{
	GENERATED_BODY()
	int32 Slot = INDEX_NONE;
};

/** Shared fragments used in the Mass Train Example */
USTRUCT()
struct ROGUEMASSEXAMPLE_API FRogueTrackSharedFragment : public FMassSharedFragment
{
	GENERATED_BODY()
	
	TWeakObjectPtr<USplineComponent> Spline;
	TArray<TPair<float, FMassEntityHandle>> StationEntities;
	TArray<FRoguePlatformData> Platforms;
	float TrackLength = 100000.f;

	FORCEINLINE bool IsValid() const { return Spline.IsValid() && TrackLength > 0.f && StationEntities.Num() == Platforms.Num(); }
	FORCEINLINE FMassEntityHandle GetStationEntityByIndex(const int32 Index) const
	{
		return StationEntities.IsValidIndex(Index) ? StationEntities[Index].Value : FMassEntityHandle();
	}
	float GetStationAlphaByIndex(const int32 Index) const;
};

USTRUCT()
struct ROGUEMASSEXAMPLE_API FRogueTrainLineSharedFragment : public FMassSharedFragment
{
	GENERATED_BODY()
	
	float CruiseSpeed = 1200.f;
	float ApproachSpeed = 500.f;
	float StopRadius = 600.f;
	float CarriageSpacingMeters = 8.f;
};

USTRUCT()
struct ROGUEMASSEXAMPLE_API FRogueDemoLimitsSharedFragment : public FMassSharedFragment
{
	GENERATED_BODY()
	
	int32 MaxPassengersOverall = 500;
	float MaxDwellTimeSeconds = 12.f;
	int32 MaxPerCarriage = 20;
};


struct FRoguePlacedCar
{
	float Alpha;
	FTransform Transform;
};