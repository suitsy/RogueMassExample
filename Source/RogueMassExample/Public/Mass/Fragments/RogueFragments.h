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
USTRUCT() struct ROGUEMASSEXAMPLE_API FRoguePassengerBoardingTag : public FMassTag { GENERATED_BODY() };
USTRUCT() struct ROGUEMASSEXAMPLE_API FRoguePassengerRidingTag : public FMassTag { GENERATED_BODY() };
USTRUCT() struct ROGUEMASSEXAMPLE_API FRoguePassengerAlightingTag: public FMassTag { GENERATED_BODY() };
USTRUCT() struct ROGUEMASSEXAMPLE_API FRoguePassengerDespawnTag : public FMassTag { GENERATED_BODY() };
USTRUCT() struct ROGUEMASSEXAMPLE_API FRogueInPoolTag : public FMassTag { GENERATED_BODY() };
USTRUCT() struct ROGUEMASSEXAMPLE_API FRoguePooledEntityTag : public FMassTag { GENERATED_BODY() };


USTRUCT()
struct ROGUEMASSEXAMPLE_API FRogueSplineFollowFragment : public FMassFragment
{
	GENERATED_BODY()
	
	float Alpha = 0.f; // [0..1] along spline
	float Speed = 0.f;  // cm/s
	FVector WorldPos = FVector::ZeroVector;
	FVector WorldFwd = FVector::ForwardVector;
};

USTRUCT()
struct ROGUEMASSEXAMPLE_API FRogueStationRefFragment : public FMassFragment
{
	GENERATED_BODY()
	
	int32 StationIndex = INDEX_NONE; // for stations/self; for passengers current/nearest
};

USTRUCT()
struct ROGUEMASSEXAMPLE_API FRogueTrainStateFragment : public FMassFragment
{
	GENERATED_BODY()
	
	bool bIsStopping = false;
	bool bAtStation = false;
	float DwellTimer = 0.f;
	float DwellTimeRemaining = 0.f;  
	float PrevAlpha = 0.f;  
	int32 TargetStationIndex = INDEX_NONE;
	int32 PreviousStationIndex = INDEX_NONE;
};

USTRUCT()
struct ROGUEMASSEXAMPLE_API FRogueTrainLinkFragment : public FMassFragment
{
	GENERATED_BODY()
	
	FMassEntityHandle LeadHandle;
	int32 CarriageIndex = 0; // 0 reserved for lead
	float SpacingMeters = 8.f;
};

USTRUCT()
struct ROGUEMASSEXAMPLE_API FRogueCarriageCapacityFragment : public FMassFragment
{
	GENERATED_BODY()
	
	int32 Capacity = 0;
	int32 Occupied = 0;
};

USTRUCT()
struct ROGUEMASSEXAMPLE_API FRoguePassengerRouteFragment : public FMassFragment
{
	GENERATED_BODY()
	
	int32 OriginStationIndex = INDEX_NONE;
	int32 DestStationIndex   = INDEX_NONE;
	int32 WaitingPointIdx    = INDEX_NONE;
	int32 CarriageIndex      = INDEX_NONE; // chosen door/carriage
};

USTRUCT()
struct ROGUEMASSEXAMPLE_API FRoguePassengerHandleFragment : public FMassFragment
{
	GENERATED_BODY()
	
	FMassEntityHandle RidingTrain;
};

USTRUCT()
struct ROGUEMASSEXAMPLE_API FRoguePassengerQueueEntry
{
	GENERATED_BODY()

	// Required
	FMassEntityHandle Passenger = FMassEntityHandle();

	// Useful metadata
	int32 DestStationIndex = INDEX_NONE;
	int32 WaitingPointIdx  = INDEX_NONE;

	// Nice-to-have for fairness/metrics
	float EnqueuedGameTime = 0.f;   // UWorld->GetTimeSeconds()
	int32 Priority         = 0;     // 0=normal, higher = board first
};

USTRUCT()
struct ROGUEMASSEXAMPLE_API FRogueStationQueueFragment : public FMassFragment
{
	GENERATED_BODY()
	TArray<FRoguePassengerQueueEntry> Queue; 
	TArray<FVector> WaitingPoints;
	TArray<FVector> SpawnPoints;   
};

/** Shared fragments used in the Mass Train Example */
USTRUCT()
struct ROGUEMASSEXAMPLE_API FRogueTrackSharedFragment : public FMassSharedFragment
{
	GENERATED_BODY()
	
	TWeakObjectPtr<USplineComponent> Spline;
	TArray<float> StationTrackAlphas; // sorted [0..1]
	TArray<FVector> StationWorldPositions;
	float TrackLength = 100000.f;  

	bool IsValid() const { return Spline.IsValid() && TrackLength > 0.f && StationTrackAlphas.Num() >= 2; }
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