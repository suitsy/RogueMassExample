// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "Kismet/KismetMathLibrary.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "CoreMinimal.h"
#include "GameplayDebuggerCategory.h"
#include "GameplayDebuggerTypes.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"

#define GPD_COND_STRING(Cond, A, B) (Cond ? TEXT("{green}" A) : TEXT("{red}" B))
#define GPD_BOOL_TO_STRING(b) (b ? TEXT("{green}On") : TEXT("{red}Off"))
#define GPD_BOOL_TO_STRING_INVERSE(b) (b ? TEXT("{red}On") : TEXT("{green}Off"))
#define GPD_BOOL_STRING_YESNO(C) (C ? TEXT("{green}Yes") : TEXT("{red}No"))

inline FColor GPDGetTempColorFromScalar(float Value, float MaxValue) {
	return FColor::MakeRedToGreenColorFromScalar(1 - UKismetMathLibrary::NormalizeToRange(Value, 0.f, MaxValue));
}
inline FString GPDGetTempColorStringFromScalar(float Value, float MaxValue) {
	if (MaxValue <= 0)
		return FString::Printf(TEXT("{Grey}%.1f"), Value);

	return FString::Printf(TEXT("{%s}%.1f{white}/{Grey}%.1f"), *GPDGetTempColorFromScalar(Value, MaxValue).ToString(), Value, MaxValue);
}
inline FString GPDGetTempColorStringFromScalar(int Value, int MaxValue) {
	if (MaxValue <= 0)
		return FString::Printf(TEXT("{Grey}%d"), Value);

	return FString::Printf(TEXT("{%s}%d{white}/{Grey}%d"), *GPDGetTempColorFromScalar(Value, MaxValue).ToString(), Value, MaxValue);
}
inline FString GPDGetTempColorStringFromPercentage(int Value) {
	return FString::Printf(TEXT("{%s}%d{white}%%"), *GPDGetTempColorFromScalar(Value, 100).ToString(), Value);
}

enum class EGameplayDebuggerEntityOverheadPairType
{
	RegularKeyValue,
	Separator
};

struct FGameplayDebuggerEntityOverheadPair
{
	FString Label;
	FString Value;
	EGameplayDebuggerEntityOverheadPairType Type = EGameplayDebuggerEntityOverheadPairType::RegularKeyValue;
	int IndentLevel                        = 0;

	FGameplayDebuggerEntityOverheadPair() = default;

	FGameplayDebuggerEntityOverheadPair(
		const FString& InLabel,
		const FString& InValue,
		EGameplayDebuggerEntityOverheadPairType InType = EGameplayDebuggerEntityOverheadPairType::RegularKeyValue,
		int InIndentLevel                        = 0
	):
		Label(InLabel),
		Value(InValue),
		Type(InType),
		IndentLevel(InIndentLevel) {}

	friend FArchive& operator<<(FArchive& Ar, FGameplayDebuggerEntityOverheadPair& Pair) {
		Ar << Pair.Label;
		Ar << Pair.Value;
		Ar << Pair.Type;
		Ar << Pair.IndentLevel;
		return Ar;
	}
};

class FGameplayDebuggerEntityOverheadCategory
{
public:
	class FGameplayDebuggerEntityOverheadTiles* Info = nullptr;

	FString Label;
	TArray<FGameplayDebuggerEntityOverheadPair> Pairs;
	int ActiveIndentLevel = 0;

	FGameplayDebuggerEntityOverheadCategory() = default;

	FGameplayDebuggerEntityOverheadCategory(FGameplayDebuggerEntityOverheadTiles* InInfo, const FString& InLabel)
		: Info(InInfo),
		  Label(InLabel) {}

	template <typename FmtType, typename... Types>
	FGameplayDebuggerEntityOverheadCategory& AddF(const FString& InLabel, const FmtType& Fmt, Types... Args) {
		static_assert(TIsArrayOrRefOfTypeByPredicate<FmtType, TIsCharEncodingCompatibleWithTCHAR>::Value, "Formatting string must be a TCHAR array.");
		static_assert((TIsValidVariadicFunctionArg<Types>::Value && ...), "Invalid argument(s) passed to FGameplayDebuggerCanvasContext::PrintfAt");

		this->AddImpl(InLabel, (const TCHAR*)Fmt, Args...);

		return *this;
	}

	FGameplayDebuggerEntityOverheadCategory& Add(const FString& InLabel, const FString& Value = "") {
		Pairs.Add(FGameplayDebuggerEntityOverheadPair{InLabel, Value, EGameplayDebuggerEntityOverheadPairType::RegularKeyValue, ActiveIndentLevel});
		return *this;
	}
	FGameplayDebuggerEntityOverheadCategory& Separator() {
		Pairs.Add(FGameplayDebuggerEntityOverheadPair{"", "", EGameplayDebuggerEntityOverheadPairType::Separator, ActiveIndentLevel});
		return *this;
	}
	FGameplayDebuggerEntityOverheadCategory& Indent() {
		ActiveIndentLevel++;
		return *this;
	}
	FGameplayDebuggerEntityOverheadCategory& Unindent() {
		ActiveIndentLevel--;
		return *this;
	}
	FGameplayDebuggerEntityOverheadCategory& operator<<(FGameplayDebuggerEntityOverheadPair&& Pair) {
		Pairs.Add(Pair);
		return *this;
	}

	void Serialize(FArchive& Ar);

private:
	void AddImpl(const FString& Label, const TCHAR* Fmt, ...);
};

struct FGameplayDebuggerEntityOverheadCategoryIndentScope
{
	FGameplayDebuggerEntityOverheadCategory& Category;

	FGameplayDebuggerEntityOverheadCategoryIndentScope(FGameplayDebuggerEntityOverheadCategory& InCategory)
		: Category(InCategory) {
		Category.Indent();
	}
	~FGameplayDebuggerEntityOverheadCategoryIndentScope() {
		Category.Unindent();
	}
};

class FGameplayDebuggerEntityOverheadTiles
{
	friend class FGameplayDebuggerEntityOverheadTilesCollector;

	class FGameplayDebuggerEntityOverheadTilesCollector* Collector = nullptr;
	class FGameplayDebuggerCategory* CategoryInst            = nullptr;

	FVector WorldPos;
	FVector2D ScreenPos;

	FGameplayDebuggerCanvasContext* CanvasContext;

	TArray<FGameplayDebuggerEntityOverheadCategory> Categories;

public:
	FGameplayDebuggerEntityOverheadTiles() = default;

	FGameplayDebuggerEntityOverheadTiles(
		class FGameplayDebuggerCategory* CategoryInst,
		const FVector& InWorldPos,
		FGameplayDebuggerCanvasContext* InCanvasContext
	);

	FGameplayDebuggerEntityOverheadTiles(
		class FGameplayDebuggerEntityOverheadTilesCollector* Collector,
		const FVector& InWorldPos
	);

	FGameplayDebuggerEntityOverheadCategory& Category(const FString& Label);

	void Print();
	void Print(float ScreenX, float ScreenY);

	void Serialize(FArchive& Ar);
};


class FGameplayDebuggerEntityOverheadTilesCollector
{
public:
	FGameplayDebuggerCategory* Category = nullptr;
	FGameplayDebuggerCanvasContext* CanvasContext = nullptr;
	FGameplayDebuggerCanvasContext WorldContext = {};

	TArray<FGameplayDebuggerEntityOverheadTiles> WorldTiles;

	FGameplayDebuggerEntityOverheadTilesCollector() = default;
	FGameplayDebuggerEntityOverheadTilesCollector(FGameplayDebuggerCategory* CategoryInst): Category(CategoryInst) {}

	FGameplayDebuggerEntityOverheadTilesCollector(
		FGameplayDebuggerCategory* CategoryInst,
		FGameplayDebuggerCanvasContext* InCanvasContext
	):
		Category(CategoryInst),
		CanvasContext(InCanvasContext) {

		if (CanvasContext) {
			WorldContext = FGameplayDebuggerCanvasContext(*CanvasContext);
			WorldContext.Font = GEngine->GetSmallFont();
			WorldContext.FontRenderInfo.bEnableShadow = true;
		}
	}

	FGameplayDebuggerEntityOverheadTiles& Add(const FVector& WorldPos);

	void Draw();

	void Serialize(FArchive& Ar);
	void From(const FGameplayDebuggerEntityOverheadTilesCollector& Collector);
};

#endif
