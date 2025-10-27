// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassCommonTypes.h"
#include "MassEntityHandle.h"
#include "Mass/Fragments/RogueFragments.h"
#include "RogueEntityDebugData.generated.h"

USTRUCT()
struct FRogueDebugMove
{
	GENERATED_BODY()

	FVector Target = FVector::ZeroVector;
	float DistToGoal = 0.f;
	FMassInt16Real DesiredSpeed = FMassInt16Real(0.f);
	float AcceptanceRadius = 0.f;
};

USTRUCT()
struct FRogueDebugTrack
{
	GENERATED_BODY()

	FVector WorldPos = FVector::ZeroVector;
	float TrackLength = 0.f;
	int32 NumStations = 0;
	TArray<float> StationAlphas;
};

USTRUCT()
struct FRogueDebugTrain
{
	GENERATED_BODY()

	// Identity
	FMassEntityHandle Entity;

	// Motion / path
	float Alpha = 0.f;          // [0..1]
	float Speed = 0.f;          // cm/s
	FVector WorldPos = FVector::ZeroVector;

	// Ops state
	bool bIsStopping = false;
	bool bAtStation = false;
	int32 TargetStationIdx = INDEX_NONE;
	float StationTimeRemaining = 0.f;
	ERogueStationTrainPhase TrainPhase = ERogueStationTrainPhase::NotStopped;
	float HeadwaySpeedScale = 1.f;
};

USTRUCT()
struct FRogueDebugCarriage
{
	GENERATED_BODY()

	// Identity
	FMassEntityHandle Entity;
	int32 IndexInTrain = 0;
	FMassEntityHandle LeadHandle; // engine

	// State
	int32 Capacity = 0;
	int32 Occupants = 0;
	float Spacing = 0;

	// Path sample (optional)
	float Alpha = 0.f;
	float Speed = 0.f;
	FVector WorldPos = FVector::ZeroVector;
};

USTRUCT()
struct FRogueDebugPassenger
{
	GENERATED_BODY()

	// Identity
	FMassEntityHandle Entity;

	// Trip
	FMassEntityHandle OriginStation;
	FMassEntityHandle DestStation;

	// Waiting/grid
	int32 WaitingPointIdx = INDEX_NONE;
	int32 WaitingSlotIdx  = INDEX_NONE;
	bool  bWaiting        = false;

	// Vehicle
	FMassEntityHandle Vehicle;  // carriage if riding

	// Movement
	ERoguePassengerPhase Phase = ERoguePassengerPhase::Pool;
	FVector WorldPos = FVector::ZeroVector;
	FRogueDebugMove Move;
};

USTRUCT()
struct FRogueDebugWaitingGrid
{
	GENERATED_BODY()

	int32 WaitingPointIdx = INDEX_NONE;
	int32 Slots = 0;
	int32 Occupied = 0;
	int32 Free = 0;
};

USTRUCT()
struct FRogueDebugStation
{
	GENERATED_BODY()

	// Identity / placement
	FMassEntityHandle Entity;
	int32 StationIdx = INDEX_NONE;
	float TrackAlpha = 0.f;
	FVector WorldPos = FVector::ZeroVector;

	// Queues
	TArray<FRogueDebugWaitingGrid> Grids;
	int32 TotalWaiting = 0;
	int32 TotalSpawnPoints = 0;
	int32 TotalWaitingPoints = 0;
};
