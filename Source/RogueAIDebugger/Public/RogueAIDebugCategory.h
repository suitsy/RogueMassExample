// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "RogueAIDebuggerEntityOverheadTiles.h"
#include "Data/RogueEntityDebugData.h"

#if WITH_GAMEPLAY_DEBUGGER

#include "GameplayDebuggerCategory.h"

class URogueTrainWorldSubsystem;


class FRogueAIDebugCategory : public FGameplayDebuggerCategory
{	
public:
	FRogueAIDebugCategory();
	static TSharedRef<FGameplayDebuggerCategory> MakeInstance();

	// FGameplayDebuggerCategory
	virtual void CollectData(APlayerController* OwnerPC, AActor* DebugActor) override;
	void CollectPassengerEntityData(); 
	void CollectTrainEntityData(); 
	void CollectCarriageEntityData(); 
	void CollectStationEntityData(); 
	void CollectTrackData();
	static void GetPassengerEntityOverheadInfo(const FRogueDebugPassenger& DebugPassenger, FGameplayDebuggerEntityOverheadTiles& Info);
	static void GetTrainEntityOverheadInfo(const FRogueDebugTrain& DebugTrain, FGameplayDebuggerEntityOverheadTiles& Info);
	static void GetCarriageEntityOverheadInfo(const FRogueDebugCarriage& DebugCarriage, FGameplayDebuggerEntityOverheadTiles& Info);
	static void GetStationEntityOverheadInfo(const FRogueDebugStation& DebugStation, FGameplayDebuggerEntityOverheadTiles& Info);
	void GetTrackEntityOverheadInfo(const FRogueDebugTrack& DebugTrack, FGameplayDebuggerEntityOverheadTiles& Info) const;
	virtual void DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& CanvasContext) override;

	void OnTogglePassengerOverheads();
	void OnToggleTrainOverheads();
	void OnToggleCarriageOverheads();
	void OnToggleStationOverheads();
	void OnToggleTrackOverheads();
	void SaveAllSettings();
	void SyncAllSettings();
	void OnSettingsUpdated(UObject* Obj, struct FPropertyChangedEvent& Event);

protected:
	bool bDrawPassengerOverheads = false;
	bool bDrawTrainOverheads = false;
	bool bDrawCarriageOverheads = false;
	bool bDrawStationOverheads = false;
	bool bDrawTrackOverheads = false;

	FDelegateHandle SettingsUpdatedHandle;
	FGameplayDebuggerEntityOverheadTilesCollector Collector;

	struct FRepData
	{
		FGameplayDebuggerEntityOverheadTilesCollector Collector;
		void Serialize(FArchive& Ar);
	};

	FRepData DataPack;

	URogueTrainWorldSubsystem* RogueTrainSubsystem;
	
private:
	struct FLocalView
	{
		FString PawnName;
		FString ControllerName;
		FString ChosenAction;
		float ChosenScore = 0.f;

		TArray<TPair<FString, float>> SortedActions;
		TArray<TPair<FString, float>> Considerations; // optional
		float WorldTime = 0.f;
	} View;

private:
	FRogueDebugPassenger PassengerCached;
	FRogueDebugTrain TrainCached;
	FRogueDebugCarriage CarriageCached;
	FRogueDebugStation StationCached;
	FRogueDebugTrack TrackCached;
};

#endif // WITH_GAMEPLAY_DEBUGGER