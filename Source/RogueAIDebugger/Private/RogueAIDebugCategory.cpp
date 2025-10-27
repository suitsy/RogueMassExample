// Fill out your copyright notice in the Description page of Project Settings.

#include "RogueAIDebugCategory.h"
#include "RogueAIDebuggerSettings.h"
#include "RogueAILineBatchProxy.h"
#include "Subsystems/RogueTrainWorldSubsystem.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "GameFramework/PlayerController.h"

FRogueAIDebugCategory::FRogueAIDebugCategory():
Collector(FGameplayDebuggerEntityOverheadTilesCollector(this))
{
	bShowOnlyWithDebugActor = false;
	RogueTrainSubsystem = nullptr;
	bAllowLocalDataCollection = false;

	const auto Settings = URogueAIDebuggerSettings::Get();
	SettingsUpdatedHandle = Settings->OnSettingChanged().AddRaw(this, &FRogueAIDebugCategory::OnSettingsUpdated);
	SyncAllSettings();

	/**0**/ BindKeyPress(EKeys::Z.GetFName(), FGameplayDebuggerInputModifier::None, this, &FRogueAIDebugCategory::OnTogglePassengerOverheads, EGameplayDebuggerInputMode::Replicated);
	/**1**/ BindKeyPress(EKeys::X.GetFName(), FGameplayDebuggerInputModifier::None, this, &FRogueAIDebugCategory::OnToggleTrainOverheads, EGameplayDebuggerInputMode::Replicated);
	/**2**/ BindKeyPress(EKeys::C.GetFName(), FGameplayDebuggerInputModifier::None, this, &FRogueAIDebugCategory::OnToggleCarriageOverheads, EGameplayDebuggerInputMode::Replicated);
	/**3**/ BindKeyPress(EKeys::V.GetFName(), FGameplayDebuggerInputModifier::None, this, &FRogueAIDebugCategory::OnToggleStationOverheads, EGameplayDebuggerInputMode::Replicated);
	/**4**/ BindKeyPress(EKeys::B.GetFName(), FGameplayDebuggerInputModifier::None, this, &FRogueAIDebugCategory::OnToggleTrackOverheads, EGameplayDebuggerInputMode::Replicated);
}

TSharedRef<FGameplayDebuggerCategory> FRogueAIDebugCategory::MakeInstance()
{
	return MakeShareable(new FRogueAIDebugCategory());
}

void FRogueAIDebugCategory::CollectData(APlayerController* OwnerPC, AActor* DebugActor)
{
	if (!OwnerPC || !DebugActor) return;

	const UWorld* World = GetWorldFromReplicator();
	if (!World) return;

	Collector = FGameplayDebuggerEntityOverheadTilesCollector(this);
	RogueTrainSubsystem = World->GetSubsystem<URogueTrainWorldSubsystem>();	
	if (!RogueTrainSubsystem) return;

	CollectPassengerEntityData();
	CollectTrainEntityData();
	CollectCarriageEntityData();
	CollectStationEntityData();
	CollectTrackData();
	
	DataPack.Collector = Collector;
}

void FRogueAIDebugCategory::CollectPassengerEntityData()
{
	if (!bDrawPassengerOverheads) return;

	const TArray<FRogueDebugPassenger>& PassengerDebugSnapshot = RogueTrainSubsystem->GetPassengerDebugSnapshot();
	for (const FRogueDebugPassenger& DebugPassenger : PassengerDebugSnapshot)
	{
		const FVector EntityPosition = DebugPassenger.WorldPos;
		FGameplayDebuggerEntityOverheadTiles& Info = Collector.Add(EntityPosition);
		GetPassengerEntityOverheadInfo(DebugPassenger, Info);
	}	
}

void FRogueAIDebugCategory::CollectTrainEntityData()
{
	if (!bDrawTrainOverheads) return;
	
	const TArray<FRogueDebugTrain>& TrainDebugSnapshot = RogueTrainSubsystem->GetTrainDebugSnapshot();
	for (const FRogueDebugTrain& DebugTrain : TrainDebugSnapshot)
	{
		const FVector EntityPosition = DebugTrain.WorldPos;
		FGameplayDebuggerEntityOverheadTiles& Info = Collector.Add(EntityPosition);
		GetTrainEntityOverheadInfo(DebugTrain, Info);
	}	
}

void FRogueAIDebugCategory::CollectCarriageEntityData()
{
	if (!bDrawCarriageOverheads) return;
	
	const TArray<FRogueDebugCarriage>& CarriageDebugSnapshot = RogueTrainSubsystem->GetCarriageDebugSnapshot();
	for (const FRogueDebugCarriage& DebugCarriage : CarriageDebugSnapshot)
	{
		const FVector EntityPosition = DebugCarriage.WorldPos;
		FGameplayDebuggerEntityOverheadTiles& Info = Collector.Add(EntityPosition);
		GetCarriageEntityOverheadInfo(DebugCarriage, Info);
	}	
}

void FRogueAIDebugCategory::CollectStationEntityData()
{
	if (!bDrawStationOverheads) return;
	
	const TArray<FRogueDebugStation>& StationDebugSnapshot = RogueTrainSubsystem->GetStationDebugSnapshot();
	for (const FRogueDebugStation& DebugStation : StationDebugSnapshot)
	{
		const FVector EntityPosition = DebugStation.WorldPos;
		FGameplayDebuggerEntityOverheadTiles& Info = Collector.Add(EntityPosition);
		GetStationEntityOverheadInfo(DebugStation, Info);
	}	
}

void FRogueAIDebugCategory::CollectTrackData()
{
	if (!bDrawTrackOverheads) return;
	
	const TArray<FRogueDebugTrack>& TrackDebugSnapshot = RogueTrainSubsystem->GetTrackDebugSnapshot();
	for (const FRogueDebugTrack& DebugTrack : TrackDebugSnapshot)
	{
		const FVector EntityPosition = DebugTrack.WorldPos;
		FGameplayDebuggerEntityOverheadTiles& Info = Collector.Add(EntityPosition);
		GetTrackEntityOverheadInfo(DebugTrack, Info);
	}	
}

void FRogueAIDebugCategory::GetPassengerEntityOverheadInfo(const FRogueDebugPassenger& DebugPassenger, FGameplayDebuggerEntityOverheadTiles& Info)
{
	FGameplayDebuggerEntityOverheadCategory& TravelCategory = Info.Category("Travel Info");
	TravelCategory.Add("Origin", FString::FromInt(DebugPassenger.OriginStation.Index));
	TravelCategory.Add("Dest", FString::FromInt(DebugPassenger.DestStation.Index));
	TravelCategory.Add("Phase", UEnum::GetDisplayValueAsText(DebugPassenger.Phase).ToString());

	FGameplayDebuggerEntityOverheadCategory& WaitingCategory = Info.Category("Wait Info");
	WaitingCategory.Add("IsWaiting", FString::Printf(TEXT("%s"), DebugPassenger.bWaiting ? TEXT("true") : TEXT("false")));
	WaitingCategory.Add("Wait", FString::FromInt(DebugPassenger.WaitingPointIdx));
	WaitingCategory.Add("WaitSlot", FString::FromInt(DebugPassenger.WaitingSlotIdx));
	
	FGameplayDebuggerEntityOverheadCategory& MovementCategory = Info.Category("Movement Info");
	MovementCategory.Add("DistToGoal", FString::Printf(TEXT("%.0f"), DebugPassenger.Move.DistToGoal)); 
	MovementCategory.Add("Speed", FString::Printf(TEXT("%.0f"), DebugPassenger.Move.DesiredSpeed.Get())); 
	MovementCategory.Add("AcceptRad", FString::Printf(TEXT("%.0f"), DebugPassenger.Move.AcceptanceRadius)); 
}

void FRogueAIDebugCategory::GetTrainEntityOverheadInfo(const FRogueDebugTrain& DebugTrain, FGameplayDebuggerEntityOverheadTiles& Info)
{
	FGameplayDebuggerEntityOverheadCategory& MovementCategory = Info.Category("Movement Info");
	MovementCategory.Add("Alpha", FString::Printf(TEXT("%.0f"), DebugTrain.Alpha)); 
	MovementCategory.Add("Speed", FString::Printf(TEXT("%.0f"), DebugTrain.Speed));
	MovementCategory.Add("TargetStation", FString::FromInt(DebugTrain.TargetStationIdx)); 
	
	FGameplayDebuggerEntityOverheadCategory& StationCategory = Info.Category("Station Info");
	StationCategory.Add("IsStopping", FString::Printf(TEXT("%s"), DebugTrain.bIsStopping ? TEXT("true") : TEXT("false")));
	StationCategory.Add("IsAtStation", FString::Printf(TEXT("%s"), DebugTrain.bAtStation ? TEXT("true") : TEXT("false")));
	StationCategory.Add("RemTime", FString::Printf(TEXT("%.0f"), DebugTrain.StationTimeRemaining));
	StationCategory.Add("StopPhase", UEnum::GetDisplayValueAsText(DebugTrain.TrainPhase).ToString());
}

void FRogueAIDebugCategory::GetCarriageEntityOverheadInfo(const FRogueDebugCarriage& DebugCarriage, FGameplayDebuggerEntityOverheadTiles& Info)
{
	FGameplayDebuggerEntityOverheadCategory& CarriageCategory = Info.Category("Carriage Info");	
	CarriageCategory.Add("Index", FString::FromInt(DebugCarriage.IndexInTrain));
	CarriageCategory.Add("Capacity", FString::FromInt(DebugCarriage.Capacity));
	CarriageCategory.Add("Occupants", FString::FromInt(DebugCarriage.Occupants));
	CarriageCategory.Add("Spacing", FString::Printf(TEXT("%.0f"), DebugCarriage.Spacing));

	FGameplayDebuggerEntityOverheadCategory& MovementCategory = Info.Category("Movement Info");
	MovementCategory.Add("Alpha", FString::Printf(TEXT("%.0f"), DebugCarriage.Alpha)); 
	MovementCategory.Add("Speed", FString::Printf(TEXT("%.0f"), DebugCarriage.Speed));
}

void FRogueAIDebugCategory::GetStationEntityOverheadInfo(const FRogueDebugStation& DebugStation, FGameplayDebuggerEntityOverheadTiles& Info)
{
	FGameplayDebuggerEntityOverheadCategory& StationCategory = Info.Category("Station Info");	
	StationCategory.Add("Index", FString::FromInt(DebugStation.StationIdx)); 	
	StationCategory.Add("Alpha", FString::Printf(TEXT("%.0f"), DebugStation.TrackAlpha)); 
	StationCategory.Add("PSpawns", FString::FromInt(DebugStation.TotalSpawnPoints)); 	

	FGameplayDebuggerEntityOverheadCategory& WaitingCategory = Info.Category("Wait Info");
	WaitingCategory.Add("WaitingCount", FString::FromInt(DebugStation.TotalWaiting)); 	
	WaitingCategory.Add("WaitPoints", FString::FromInt(DebugStation.TotalWaitingPoints));

	for (int i = 0; i < DebugStation.Grids.Num(); ++i)
	{
		const FRogueDebugWaitingGrid& WaitGrid = DebugStation.Grids[i];
		WaitingCategory.Add(FString::Printf(TEXT("Grid%d"), i),FString::Printf(TEXT("WP %d | Slots %d | Occ %d | Free %d"),
				  WaitGrid.WaitingPointIdx, WaitGrid.Slots, WaitGrid.Occupied, WaitGrid.Free));
		/*WaitingCategory.Add("WpIndex", FString::FromInt(WaitGrid.WaitingPointIdx));
		WaitingCategory.Add("Slots", FString::FromInt(WaitGrid.Slots));
		WaitingCategory.Add("Occupied", FString::FromInt(WaitGrid.Occupied));
		WaitingCategory.Add("Free", FString::FromInt(WaitGrid.Free));
		WaitingCategory.Add("--", FString::Printf(TEXT("%s"), TEXT("----")));*/
	}
}

void FRogueAIDebugCategory::GetTrackEntityOverheadInfo(const FRogueDebugTrack& DebugTrack, FGameplayDebuggerEntityOverheadTiles& Info) const
{
	
}

void FRogueAIDebugCategory::DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& Context)
{
	Context.Printf(TEXT("[{yellow}%s{white}] Passenger Overheads: %s"), *GetInputHandlerDescription(0), GPD_COND_STRING(bDrawPassengerOverheads, "On", "Off"));
	Context.Printf(TEXT("[{yellow}%s{white}] Train Overheads: %s"), *GetInputHandlerDescription(1), GPD_COND_STRING(bDrawTrainOverheads, "On", "Off"));
	Context.Printf(TEXT("[{yellow}%s{white}] Carriage Overheads: %s"), *GetInputHandlerDescription(3), GPD_COND_STRING(bDrawCarriageOverheads, "On", "Off"));
	Context.Printf(TEXT("[{yellow}%s{white}] Station Overheads: %s"), *GetInputHandlerDescription(4), GPD_COND_STRING(bDrawStationOverheads, "On", "Off"));
	Context.Printf(TEXT("[{yellow}%s{white}] Track Overheads: %s"), *GetInputHandlerDescription(5), GPD_COND_STRING(bDrawTrackOverheads, "On", "Off"));

	Context.MoveToNewLine();

	FVector ViewLocation  = FVector::ZeroVector;
	FVector ViewDirection = FVector::ZeroVector;
	GetViewPoint(OwnerPC, ViewLocation, ViewDirection);

	FLineBatchProxy Drawer(GetWorldFromReplicator());
	Drawer.Thickness(4.0f);

	Collector = FGameplayDebuggerEntityOverheadTilesCollector(this, &Context);
	Collector.From(DataPack.Collector);
	Collector.Draw();
	
	/*if (!Cached.Owner.IsValid())
	{
		Context.Printf(TEXT("{gray}RogueAI: no data"));
		return;
	}

	Context.Printf(TEXT("{white}RogueAI — %s  {gray}(t=%.2f)"), *GetNameSafe(Cached.Owner.Get()), Cached.TimeStamp);

	for (const FRogueAIDebugChannelEntry& Row : Cached.Channels)
	{
		Context.Printf(TEXT("{yellow}%s {white}→ {cyan}%s {white}Score:{green}%.2f {gray}CD:{white}%.2fs %s"),
			*Row.Channel.ToString(),
			*Row.ActionName,
			Row.Score,
			Row.CooldownRemaining,
			Row.PayloadHint.Len() ? *FString::Printf(TEXT("{gray}[%s]"), *Row.PayloadHint) : TEXT(""));
	}
	
	for (const FChannelDebugRow& CH : Cached.ChannelsDetailed)
	{
		Context.Printf(TEXT("{yellow}%s"), *CH.Channel.ToString());
		for (int32 idx=0; idx<CH.Actions.Num(); ++idx)
		{
			const FActionDebugRow& A = CH.Actions[idx];
			const bool bPicked = (idx == CH.PickedIndex);
			Context.Printf(TEXT("  %s %s Score=%.2f%s"),
				bPicked?TEXT("{green}*"):TEXT(" "),
				*A.ActionName, A.Score, A.bVeto?TEXT(" VETO"):TEXT(""));

			for (const FConsiderationDebugRow& C : A.Considerations)
			{
				Context.Printf(TEXT("{green}Consideration:"));
				Context.Printf(TEXT("{gray}%s Raw=%.2f Invert=%d Used={white}%.2f%s"),
					*C.ConsName, C.Raw, C.bInvert?1:0, C.Used, C.bVeto?TEXT(" VETO"):TEXT(""));

				for (const FServiceDebugRow& S : C.Services)
				{
					Context.Printf(TEXT("{green}Services:"));
					Context.Printf(TEXT("{gray}- %s Base=%.2f Gate=%.0f Final=%.2f %s"),
						*S.ServiceName, S.Base, S.Gate, S.Final, *S.ValuePreview);
				}
			}
		}
	}*/
}

void FRogueAIDebugCategory::OnTogglePassengerOverheads()
{
	bDrawPassengerOverheads = !bDrawPassengerOverheads;
	SaveAllSettings();
}

void FRogueAIDebugCategory::OnToggleTrainOverheads()
{
	bDrawTrainOverheads = !bDrawTrainOverheads;
	SaveAllSettings();
}

void FRogueAIDebugCategory::OnToggleCarriageOverheads()
{
	bDrawCarriageOverheads = !bDrawCarriageOverheads;
	SaveAllSettings();
}

void FRogueAIDebugCategory::OnToggleStationOverheads()
{
	bDrawStationOverheads = !bDrawStationOverheads;
	SaveAllSettings();
}

void FRogueAIDebugCategory::OnToggleTrackOverheads()
{
	bDrawTrackOverheads = !bDrawTrackOverheads;
	SaveAllSettings();
}

void FRogueAIDebugCategory::SaveAllSettings()
{
	auto Settings = URogueAIDebuggerSettings::Get();
	Settings->GameplayDebuggerConfig.bDrawPassengerOverheads = bDrawPassengerOverheads;
	Settings->GameplayDebuggerConfig.bDrawTrainOverheads = bDrawTrainOverheads;
	Settings->GameplayDebuggerConfig.bDrawCarriageOverheads = bDrawCarriageOverheads;
	Settings->GameplayDebuggerConfig.bDrawStationOverheads = bDrawStationOverheads;
	Settings->GameplayDebuggerConfig.bDrawTrackOverheads = bDrawTrackOverheads;

	Settings->SaveConfig();
	Settings->TryUpdateDefaultConfigFile();
}

void FRogueAIDebugCategory::SyncAllSettings()
{
	auto Conf = URogueAIDebuggerSettings::Get()->GameplayDebuggerConfig;
	bDrawPassengerOverheads = Conf.bDrawPassengerOverheads;
	bDrawTrainOverheads = Conf.bDrawTrainOverheads;
	bDrawStationOverheads = Conf.bDrawStationOverheads;
}

void FRogueAIDebugCategory::OnSettingsUpdated(UObject* Obj, struct FPropertyChangedEvent& Event)
{
	SyncAllSettings();
}

#endif 
