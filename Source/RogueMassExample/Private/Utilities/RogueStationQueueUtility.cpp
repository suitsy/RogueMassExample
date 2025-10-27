// Fill out your copyright notice in the Description page of Project Settings.


#include "Utilities/RogueStationQueueUtility.h"
#include "MassEntityManager.h"


void RogueStationQueueUtility::BuildGridForWaitingPoint(const FRoguePlatformData& StationSegment, FRogueStationQueueFragment& QueueFragment,
                                                        const FVector& WaitingCenter, const int32 WaitingPointIdx)
{
	FRogueWaitingGrid& Grid = QueueFragment.Grids.FindOrAdd(WaitingPointIdx);
	Grid.SlotPositions.Reset();
	Grid.OccupiedBy.Reset();

	const int32 Cols = FMath::Max(1, QueueFragment.WaitingGridConfig.GridCols);
	const int32 Rows = FMath::Max(1, QueueFragment.WaitingGridConfig.GridRows);
	const float ColumnHalfWidth = 0.5f * (Cols - 1);
	const float RowHalfWidth = 0.5f * (Rows - 1);
	const float MaxHalfWidth = 0.5f * StationSegment.PlatformLength - StationSegment.WaitingGridConfig.GridEdgeInset;
	const FVector GridCenter = WaitingCenter + StationSegment.Right * StationSegment.WaitingGridConfig.GridOffset;
	
	for (int32 Row = 0; Row < Rows; ++Row)
	{
		for (int32 Col = 0; Col < Cols; ++Col)
		{
			const float Length = (Col - ColumnHalfWidth) * QueueFragment.WaitingGridConfig.GridColSpacing;
			const float Width = (Row - RowHalfWidth) * QueueFragment.WaitingGridConfig.GridRowSpacing;
			
			const float ClampedLength = FMath::Clamp(Length, -MaxHalfWidth, MaxHalfWidth);
			const FVector PointAdjusted = GridCenter + StationSegment.Fwd * ClampedLength + StationSegment.Right * (Width + 20.f);

			Grid.SlotPositions.Add(PointAdjusted);
		}
	}

	Grid.OccupiedBy.Init(FMassEntityHandle(), Grid.SlotPositions.Num());
	Grid.FreeIndices.Reset();
}

int32 RogueStationQueueUtility::ClaimWaitingSlot(FRogueStationQueueFragment* QueueFragment, const int32 WaitingPointIdx, const FMassEntityHandle& Passenger, FVector& OutSlotPos)
{
	FRogueWaitingGrid* Grid = QueueFragment->Grids.Find(WaitingPointIdx);
	if (!Grid) return INDEX_NONE;

	// Fast path: free list
	while (!Grid->FreeIndices.IsEmpty())
	{
		const int32 Idx = Grid->FreeIndices.Pop(EAllowShrinking::No);
		if (Grid->OccupiedBy.IsValidIndex(Idx) && !Grid->OccupiedBy[Idx].IsValid())
		{
			Grid->OccupiedBy[Idx] = Passenger;
			OutSlotPos = Grid->SlotPositions[Idx];
			return Idx;
		}
	}

	int32 SlotIdx = INDEX_NONE;
	int32 CheckCount = Grid->OccupiedBy.Num();
	while (SlotIdx == INDEX_NONE)
	{
		const int32 TestIdx = (Grid->OccupiedBy.Num() > 0) ? FMath::RandRange(0, Grid->OccupiedBy.Num() - 1) : INDEX_NONE;
		if (TestIdx != INDEX_NONE)
		{
			// Check slot is free
			if (!Grid->OccupiedBy[TestIdx].IsValid())
			{
				SlotIdx = TestIdx;
				Grid->OccupiedBy[SlotIdx] = Passenger;
				OutSlotPos = Grid->SlotPositions[SlotIdx];
				return SlotIdx;
			}
		}

		CheckCount--;
		if (CheckCount <= 0) break;
	}
	
	// Fallback: first available
	for (int32 i = 0; i < Grid->OccupiedBy.Num(); ++i)
	{
		if (!Grid->OccupiedBy[i].IsValid())
		{
			Grid->OccupiedBy[i] = Passenger;
			OutSlotPos = Grid->SlotPositions[i];
			return i;
		}
	}
	return INDEX_NONE;
}

void RogueStationQueueUtility::ReleaseSlot(FRogueStationQueueFragment& QueueFragment, const FRoguePassengerFragment& PassengerFragment)
{
	if (FRogueWaitingGrid* Grid = QueueFragment.Grids.Find(PassengerFragment.WaitingPointIdx))
	{
		if (Grid->IsValidSlotIndex(PassengerFragment.WaitingSlotIdx))
		{
			Grid->OccupiedBy[PassengerFragment.WaitingSlotIdx] = FMassEntityHandle();
			Grid->FreeIndices.Add(PassengerFragment.WaitingSlotIdx);
		}
	}
}

/*bool RogueStationQueueUtility::DequeueFromGrid(const FMassEntityManager& EntityManger, FRogueStationQueueFragment& QueueFragment, const int32 WaitPointIdx,
	FMassEntityHandle& OutPassenger, int32& OutSlotIdx, FVector& OutSlotPos)
{
	FRogueWaitingGrid* Grid = QueueFragment.Grids.Find(WaitPointIdx);
	if (!Grid) return false;

	// Prefer an actually occupied slot; scan once (grids are small).
	for (int32 i = 0; i < Grid->OccupiedBy.Num(); ++i)
	{
		if (Grid->OccupiedBy[i].IsValid())
		{
			if (const auto PassengerFragment = EntityManger.GetFragmentDataPtr<FRoguePassengerFragment>(Grid->OccupiedBy[i]))
			{
				if (!PassengerFragment->bWaiting) continue;
				
				OutPassenger = Grid->OccupiedBy[i];
				OutSlotIdx = i;
				OutSlotPos = Grid->SlotPositions.IsValidIndex(i) ? Grid->SlotPositions[i] : FVector::ZeroVector;

				ReleaseSlot(QueueFragment, WaitPointIdx, OutSlotIdx);
				return true;
			}
			
		}
	}
	
	return false;
}*/

bool RogueStationQueueUtility::PeekFromGrid(const FMassEntityManager& EntityManger, FRogueStationQueueFragment& QueueFragment, const int32 WaitPointIdx,
	FMassEntityHandle& OutPassenger, const FMassEntityHandle CurrentStationEntity, int32& OutSlotIdx, FVector& OutSlotPos)
{
	FRogueWaitingGrid* Grid = QueueFragment.Grids.Find(WaitPointIdx);
	if (!Grid) return false;

	// Prefer an actually occupied slot; scan once (grids are small).
	for (int32 i = 0; i < Grid->OccupiedBy.Num(); ++i)
	{
		if (Grid->OccupiedBy[i].IsValid())
		{
			if (const auto PassengerFragment = EntityManger.GetFragmentDataPtr<FRoguePassengerFragment>(Grid->OccupiedBy[i]))
			{
				if (!PassengerFragment->bWaiting || PassengerFragment->OriginStation != CurrentStationEntity) continue;
				
				OutPassenger = Grid->OccupiedBy[i];
				OutSlotIdx = i;
				OutSlotPos = Grid->SlotPositions.IsValidIndex(i) ? Grid->SlotPositions[i] : FVector::ZeroVector;
				return true;
			}
			
		}
	}
	
	return false;
}
