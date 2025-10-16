// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettingsBackedByCVars.h"
#include "RogueDeveloperSettings.generated.h"

class UMassEntityConfigAsset;
/**
 * 
 */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Rogue Mass Example"))
class ROGUEMASSEXAMPLE_API URogueDeveloperSettings : public UDeveloperSettingsBackedByCVars
{
	GENERATED_BODY()
	
public:
	/** Number of trains to simulate */
	UPROPERTY(EditDefaultsOnly, Config, Category="Trains", meta=(ClampMin="1"))
	int32 NumTrains = 2;

	/** Number of carriages per train */
	UPROPERTY(EditDefaultsOnly, Config, Category="Trains", meta=(ClampMin="0"))
	int32 CarriagesPerTrain = 3;

	/** Maximum number of passengers per carriage */
	UPROPERTY(EditDefaultsOnly, Config, Category="Trains", meta=(ClampMin="0"))
	int32 MaxPassengersPerCarriage = 20;

	/** Spacing between carriages in meters */
	UPROPERTY(EditDefaultsOnly, Config, Category="Trains", meta=(ClampMin="1.0"))
	float CarriageSpacingMeters = 8.f;

	/** Speed of the lead carriage when cruising (not approaching a station) */
	UPROPERTY(EditDefaultsOnly, Config, Category="Trains")
	float LeadCruiseSpeed = 1200.f; // cm/s

	/** Speed of the lead carriage when approaching a station */
	UPROPERTY(EditDefaultsOnly, Config, Category="Trains")
	float StationApproachSpeed = 500.f;

	/** Acceleration and deceleration rate of the lead carriage */
	UPROPERTY(EditDefaultsOnly, Config, Category="Stations", meta=(ClampMin="0"))
	float MaxDwellTimeSeconds = 12.f;

	/** Maximum number of passengers allowed in the simulation at once */
	UPROPERTY(EditDefaultsOnly, Config, Category="Passengers", meta=(ClampMin="0"))
	int32 MaxPassengersOverall = 500;

	/** Interval between spawning new passengers */
	UPROPERTY(EditDefaultsOnly, Config, Category="Passengers", meta=(ClampMin="0"))
	float SpawnIntervalSeconds = 0.25f;

	/** Actor containing the spline component defining the track layout */
	UPROPERTY(EditDefaultsOnly, Config, Category="Track")
	TSoftObjectPtr<AActor> TrackSplineActor;

	/** Radius around a station where trains begin to slow down */
	UPROPERTY(EditDefaultsOnly, Config, Category="Stations")
	float StationStopRadius = 600.f;

	// Station / Train / Passenger templates
	UPROPERTY(EditDefaultsOnly, Config, Category="Entity Templates")
	TSoftObjectPtr<UMassEntityConfigAsset> StationConfig = nullptr;

	UPROPERTY(EditDefaultsOnly, Config, Category="Entity Templates")
	TSoftObjectPtr<UMassEntityConfigAsset> TrainEngineConfig = nullptr;

	UPROPERTY(EditDefaultsOnly, Config, Category="Entity Templates")
	TSoftObjectPtr<UMassEntityConfigAsset> TrainCarriageConfig = nullptr;

	UPROPERTY(EditDefaultsOnly, Config, Category="Entity Templates")
	TSoftObjectPtr<UMassEntityConfigAsset> PassengerConfig = nullptr;

	// Spawn throttle
	UPROPERTY(EditDefaultsOnly, Config, Category="Spawning", meta=(ClampMin="1"))
	int32 MaxSpawnsPerFrame = 64;
};
