// Fill out your copyright notice in the Description page of Project Settings.

#include "RogueAIDebuggerEntityOverheadTiles.h"
#include "CanvasItem.h"


// copied from Core/Private/Misc/VarargsHeler.h 
#define GROWABLE_PRINTF(PrintFunc) \
	int32	BufferSize	= 1024; \
	TCHAR*	Buffer		= NULL; \
	int32	Result		= -1; \
	/* allocate some stack space to use on the first pass, which matches most strings */ \
	TCHAR	StackBuffer[512]; \
	TCHAR*	AllocatedBuffer = NULL; \
\
	/* first, try using the stack buffer */ \
	Buffer = StackBuffer; \
	GET_TYPED_VARARGS_RESULT( TCHAR, Buffer, UE_ARRAY_COUNT(StackBuffer), UE_ARRAY_COUNT(StackBuffer) - 1, Fmt, Fmt, Result ); \
\
	/* if that fails, then use heap allocation to make enough space */ \
	while(Result == -1) \
	{ \
		FMemory::SystemFree(AllocatedBuffer); \
		/* We need to use malloc here directly as GMalloc might not be safe. */ \
		Buffer = AllocatedBuffer = (TCHAR*) FMemory::SystemMalloc( BufferSize * sizeof(TCHAR) ); \
		GET_TYPED_VARARGS_RESULT( TCHAR, Buffer, BufferSize, BufferSize-1, Fmt, Fmt, Result ); \
		BufferSize *= 2; \
	}; \
	Buffer[Result] = 0; \
	; \
\
	PrintFunc; \
	FMemory::SystemFree(AllocatedBuffer);


void FGameplayDebuggerEntityOverheadCategory::AddImpl(const FString& InLabel, const TCHAR* Fmt, ...) {
	GROWABLE_PRINTF(Add(InLabel, Buffer));
}

FGameplayDebuggerEntityOverheadTiles::FGameplayDebuggerEntityOverheadTiles(class FGameplayDebuggerCategory* CategoryInst, const FVector& InWorldPos, FGameplayDebuggerCanvasContext* InCanvasContext):
	CategoryInst(CategoryInst),
	WorldPos(InWorldPos),
	CanvasContext(InCanvasContext) {

	ScreenPos = CanvasContext->ProjectLocation(WorldPos);

}

FGameplayDebuggerEntityOverheadTiles::FGameplayDebuggerEntityOverheadTiles(FGameplayDebuggerEntityOverheadTilesCollector* Collector, const FVector& InWorldPos):
	Collector(Collector),
	CategoryInst(Collector->Category),
	WorldPos(InWorldPos),
	CanvasContext(&Collector->WorldContext) {

	ScreenPos = CanvasContext->ProjectLocation(WorldPos);
}

FGameplayDebuggerEntityOverheadCategory& FGameplayDebuggerEntityOverheadTiles::Category(const FString& Label) {
	return Categories.Emplace_GetRef(this, Label);
}

void FGameplayDebuggerEntityOverheadTiles::Print() {
	Print(ScreenPos.X, ScreenPos.Y);
}

void FGameplayDebuggerEntityOverheadTiles::Print(const float ScreenX, const float ScreenY) {
	FString FullOutputString = "";

	bool bHasData = false;

	int Idx = 0;
	for (FGameplayDebuggerEntityOverheadCategory& Category : Categories) {
		if (Idx > 0) {
			FullOutputString += "{(R=0,G=0,B=0,A=0)} - \n";
		}

		FullOutputString += FString::Printf(TEXT("{yellow}%s\n"), *Category.Label);

		bool bCategoryHasData = false;

		for (FGameplayDebuggerEntityOverheadPair& Pair : Category.Pairs) {
			switch (Pair.Type) {

				case EGameplayDebuggerEntityOverheadPairType::RegularKeyValue: {
					for (int i = 0; i < Pair.IndentLevel; ++i) {
						FullOutputString += "    ";
					}					
					FullOutputString += FString::Printf(TEXT("{white}%s: {silver}%s\n"), *Pair.Label, *Pair.Value);
					bHasData         = true;
					bCategoryHasData = true;
					break;
				}

				case EGameplayDebuggerEntityOverheadPairType::Separator: {
					if (bCategoryHasData)
						FullOutputString += TEXT("{DimGrey}---------------------\n");
					break;
				}
			}
			
		}

		Idx++;
	}

	if (!bHasData) {
		return;
	}

	float SizeX(0.f), SizeY(0.f);
	CanvasContext->MeasureString(FullOutputString, SizeX, SizeY);
	float PrintX = ScreenX - (SizeX * 0.5f);
	float PrintY = ScreenY - (SizeY * 1.2f);

	const FVector2D BackgroundPos  = FVector2D(PrintX - 5, PrintY - 5);
	const FVector2D BackgroundSize = FVector2D(SizeX + 10, SizeY + 10);

	FCanvasTileItem Background(FVector2D(0.0f), BackgroundSize, FLinearColor(0.1, 0.1, 0.1, 0.8));
	Background.BlendMode = SE_BLEND_Translucent;

	CanvasContext->DrawItem(Background, BackgroundPos.X, BackgroundPos.Y);
	CanvasContext->PrintAt(PrintX, PrintY, FullOutputString);
}

FGameplayDebuggerEntityOverheadTiles& FGameplayDebuggerEntityOverheadTilesCollector::Add(const FVector& WorldPos) {
	auto Tile = WorldTiles.FindByPredicate([&WorldPos](const FGameplayDebuggerEntityOverheadTiles& Tile) {
		return Tile.WorldPos == WorldPos;
	});
	if (!Tile) {
		return WorldTiles.Add_GetRef(FGameplayDebuggerEntityOverheadTiles(this, WorldPos));
	}

	return *Tile;
}

void FGameplayDebuggerEntityOverheadTilesCollector::Draw() {
	for (FGameplayDebuggerEntityOverheadTiles& Tile : WorldTiles) {
		if (!CanvasContext->IsLocationVisible(Tile.WorldPos))
			continue;

		Tile.Print(Tile.ScreenPos.X, Tile.ScreenPos.Y);
	}
}

void FGameplayDebuggerEntityOverheadTilesCollector::From(const FGameplayDebuggerEntityOverheadTilesCollector& Collector) {
	for (FGameplayDebuggerEntityOverheadTiles WorldTile : Collector.WorldTiles) {
		auto& ThisTile = Add(WorldTile.WorldPos);
		for (FGameplayDebuggerEntityOverheadCategory C : WorldTile.Categories) {
			auto& ThisCategory = ThisTile.Category(C.Label);
			for (FGameplayDebuggerEntityOverheadPair Pair : C.Pairs) {
				ThisCategory << MoveTemp(Pair);
			}
		}
	}
}
void FGameplayDebuggerEntityOverheadTilesCollector::Serialize(FArchive& Ar) {
	int32 NumTiles = WorldTiles.Num();
	Ar << NumTiles;

	if (Ar.IsLoading()) {
		WorldTiles.SetNum(NumTiles);
	}

	for (int32 Idx = 0; Idx < NumTiles; Idx++) {
		if (Ar.IsLoading()) {
			WorldTiles[Idx].Collector     = this;
			WorldTiles[Idx].CanvasContext = &WorldContext;
			WorldTiles[Idx].CategoryInst  = Category;
		}
		WorldTiles[Idx].Serialize(Ar);
	}

}

void FGameplayDebuggerEntityOverheadCategory::Serialize(FArchive& Ar) {
	Ar << Label;
	Ar << ActiveIndentLevel;

	int32 NumPairs = Pairs.Num();
	Ar << NumPairs;

	if (Ar.IsLoading()) {
		Pairs.SetNum(NumPairs);
	}

	for (int32 Idx = 0; Idx < NumPairs; Idx++) {
		Ar << Pairs[Idx];
	}
}

void FGameplayDebuggerEntityOverheadTiles::Serialize(FArchive& Ar) {
	Ar << WorldPos;
	Ar << ScreenPos;

	int32 NumCategories = Categories.Num();
	Ar << NumCategories;

	if (Ar.IsLoading()) {
		Categories.SetNum(NumCategories);
	}

	for (int32 Idx = 0; Idx < NumCategories; Idx++) {
		if (Ar.IsLoading()) {
			Categories[Idx].Info = this;
		}
		Categories[Idx].Serialize(Ar);
	}
}
