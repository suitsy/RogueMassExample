// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Mass/Fragments/RogueFragments.h"


namespace RogueStationQueueUtility
{
	void BuildGridForWaitingPoint(const FRoguePlatformData& StationSegment, FRogueStationQueueFragment& QueueFragment, const FVector& WaitingCenter, int32 WaitingPointIdx);
	int32 ClaimWaitingSlot(FRogueStationQueueFragment* QueueFragment, const int32 WaitingPointIdx, const FMassEntityHandle& Passenger, FVector& OutSlotPos);
	void ReleaseSlot(FRogueStationQueueFragment& QueueFragment, const FRoguePassengerFragment& PassengerFragment);
	//bool DequeueFromGrid(const FMassEntityManager& EntityManger, FRogueStationQueueFragment& QueueFragment, const int32 WaitPointIdx, FMassEntityHandle& OutPassenger, int32& OutSlotIdx, FVector& OutSlotPos);
	bool PeekFromGrid(const FMassEntityManager& EntityManger, FRogueStationQueueFragment& QueueFragment, const int32 WaitPointIdx,
		FMassEntityHandle& OutPassenger, const FMassEntityHandle CurrentStationEntity, int32& OutSlotIdx, FVector& OutSlotPos);
}