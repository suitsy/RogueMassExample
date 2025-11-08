// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettingsBackedByCVars.h"
#include "Mass/Fragments/RogueFragments.h"
#include "RogueDeveloperSettings.generated.h"

class UMassEntityConfigAsset;



UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Rogue Mass Example"))
class ROGUEMASSEXAMPLE_API URogueDeveloperSettings : public UDeveloperSettingsBackedByCVars
{
	GENERATED_BODY()
	
public:
	/** Simulation Settings */

	/** Actor containing the spline component defining the track layout */
	UPROPERTY(EditDefaultsOnly, Config, Category="Simulation Settings")
	TSoftObjectPtr<AActor> TrackSplineActor;

	/** Maximum number of passengers allowed in the simulation at once */
	UPROPERTY(EditDefaultsOnly, Config, Category="Simulation Settings", meta=(ClampMin="0"))
	int32 MaxPassengersOverall = 500;

	/** Interval between spawning new passengers */
	UPROPERTY(EditDefaultsOnly, Config, Category="Simulation Settings", meta=(ClampMin="0"))
	float SpawnIntervalSeconds = 0.25f;

	/** Interval between spawning new passengers */
	UPROPERTY(EditDefaultsOnly, Config, Category="Simulation Settings", meta=(ClampMin="0"))
	float TrackSplineResampleStep = 500.f;
	
	/** Maximum number of entities to spawn per frame to avoid hitches */
	UPROPERTY(EditDefaultsOnly, Config, Category="Spawning", meta=(ClampMin="1"))
	int32 MaxSpawnsPerFrame = 64;

	/** Train, Carriage and Passenger Settings */
	
	/** Number of trains to simulate */
	UPROPERTY(EditDefaultsOnly, Config, Category="Trains", meta=(ClampMin="1"))
	int32 NumTrains = 2;
	
	/** Lead engine visual length */
	UPROPERTY(EditAnywhere, Config, Category="Trains")
	float EngineLength = 2000.f;
	
	/** Height of train from ground */
	UPROPERTY(EditDefaultsOnly, Config, Category="Trains", meta=(ClampMin="1"))
	float EngineRideHeight = 10.f;

	/** MaxSpeed of the lead carriage when cruising (not approaching a station) */
	UPROPERTY(EditDefaultsOnly, Config, Category="Trains")
	float LeadCruiseSpeed = 500.f;
	
	/** MaxSpeed of the lead carriage when approaching a station */
	UPROPERTY(EditDefaultsOnly, Config, Category="Trains")
	float StationApproachSpeed = 250.f;

	/** Number of carriages per train */
	UPROPERTY(EditDefaultsOnly, Config, Category="Trains|Carriages", meta=(ClampMin="0"))
	int32 CarriagesPerTrain = 3; 

	/** Length of the carriage model */
	UPROPERTY(EditDefaultsOnly, Config, Category="Trains|Carriages", meta=(ClampMin="1.0"))
	float CarriageLength = 200.f;

	/** Carriage ride height from rail to mesh pivot */
	UPROPERTY(EditAnywhere, Config, Category="Trains|Carriages")
	float CarriageRideHeight = 10.f;

	/** Spacing between carriages */
	UPROPERTY(EditDefaultsOnly, Config, Category="Trains|Carriages", meta=(ClampMin="1.0"))
	float CarriageSpacing = 10.f; 

	/** Maximum passenger loading into carriage per tick */
	UPROPERTY(EditDefaultsOnly, Config, Category="Trains|Carriages", meta=(ClampMin="1.0"))
	float MaxLoadPerTickPerCarriage = 4.f; 

	/** Passenger unload rate */
	UPROPERTY(EditDefaultsOnly, Config, Category="Trains|Carriages", meta=(ClampMin="1.0"))
	float UnloadIntervalSeconds = 0.25f; 

	/** Passenger unload jitter */
	UPROPERTY(EditDefaultsOnly, Config, Category="Trains|Carriages", meta=(ClampMin="1.0"))
	float UnloadStartJitter = 0.15f;  

	/** Maximum number of passengers per carriage */
	UPROPERTY(EditDefaultsOnly, Config, Category="Trains|Carriages", meta=(ClampMin="0"))
	int32 MaxPassengersPerCarriage = 100;

	/** Passengers destination acceptance radius */
	UPROPERTY(EditDefaultsOnly, Config, Category="Trains|Passengers", meta=(ClampMin="0"))
	float PassengerAcceptanceRadius = 50.f;	

	/** Passengers collision radius */
	UPROPERTY(EditDefaultsOnly, Config, Category="Trains|Passengers", meta=(ClampMin="0"))
	float PassengerRadius = 35.f;	

	/** Interval between spawning new passengers */
	UPROPERTY(EditDefaultsOnly, Config, Category="Trains|Passengers", meta=(ClampMin="0"))
	float PassengerMaxSpeed = 150.f;

	/** Acceleration and deceleration rate of the lead carriage */
	UPROPERTY(EditDefaultsOnly, Config, Category="Stations", meta=(ClampMin="0"))
	float MaxDwellTimeSeconds = 15.f;

	/** Time buffer from allowing boarding to leaving the station */
	UPROPERTY(EditDefaultsOnly, Config, Category="Stations", meta=(ClampMin="0"))
	float DepartureTimeSeconds = 5.f; 	

	// Station list
	UPROPERTY(EditAnywhere, Config, Category="Stations")
	TArray<FRogueStationConfig> Stations;

	/** Radius around a station where trains begin to slow down */
	UPROPERTY(EditDefaultsOnly, Config, Category="Stations")
	float StationStopRadius = 1000.f;

	/** Radius a train will stop at a station from a docking point*/
	UPROPERTY(EditDefaultsOnly, Config, Category="Stations")
	float StationArrivalRadius = 50.f;

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

	// Debugging
	UPROPERTY(EditDefaultsOnly, Config, Category="Debug|Stations")
	bool bDrawStationSpawnPoints = false;
	
	UPROPERTY(EditDefaultsOnly, Config, Category="Debug|Stations")
	bool bDrawStationWaitPoints = false;
	
	UPROPERTY(EditDefaultsOnly, Config, Category="Debug|Stations")
	bool bDrawStationWaitGrid = false;
};
