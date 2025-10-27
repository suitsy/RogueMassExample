// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/LineBatchComponent.h"

class ULineBatchComponent;

struct ROGUEAIDEBUGGER_API FPartialLine
{
	FVector Start;
	FVector End;

	FPartialLine() = default;

	FPartialLine(const FVector& InStart, const FVector& InEnd) : Start(InStart),
	                                                             End(InEnd) {}
};

struct ROGUEAIDEBUGGER_API FLineBatchProxy
{
	virtual ~FLineBatchProxy() = default;


	static constexpr uint32 INVALID_ID = 0;

	uint32 BatchID = INVALID_ID;

	TObjectPtr<ULineBatchComponent> LineBatcher;

	/**
	 * \typedef ESceneDepthPriorityGroup
	 */
	uint8 DepthPriority = SDPG_Foreground;

	FLinearColor CurrentColor = FLinearColor::Red;

	float CurrentThickness = 4.0f;

	/**
	 * All calls are on the foreground line batcher, which does not have a lifetime.
	 * It's just here for a default for all underlying LineBatchComponent calls.
	 */
	float LifeTime = 0.0f;

	FLineBatchProxy() = default;

	explicit FLineBatchProxy(float DefaultThickness);
	explicit FLineBatchProxy(const UWorld* World, float DefaultThickness = 2.0f);
	explicit FLineBatchProxy(ULineBatchComponent* InLineBatcher, float DefaultThickness = 2.0f);

	static FLineBatchProxy MakePersistent(const UWorld* World, float InDefaultLifetime = 1.0f) {
		return FLineBatchProxy(World).Persistent(InDefaultLifetime);
	}

	FLineBatchProxy& Persistent(float InLifeTime = 0.0f);
	FLineBatchProxy& Foreground();

	FLineBatchProxy& Color(const FLinearColor& InColor);
	FLineBatchProxy& Thickness(float InThickness);

	struct ScopedColor
	{
		FLineBatchProxy& Proxy;
		FLinearColor CurrentColor;
		FLinearColor OldColor;

		ScopedColor(FLineBatchProxy& InProxy, const FLinearColor& InColor) :
			Proxy(InProxy),
			CurrentColor(InColor),
			OldColor(InProxy.CurrentColor) {
			Proxy.Color(InColor);
		}

		~ScopedColor() { Proxy.Color(OldColor); }
	};

	ScopedColor ColorTemp(const FLinearColor& InColor) { return ScopedColor(*this, InColor); }

	struct ScopedThickness
	{
		FLineBatchProxy& Proxy;
		float OldThickness;

		ScopedThickness(FLineBatchProxy& InProxy, float InThickness)
			: Proxy(InProxy),
			  OldThickness(InProxy.CurrentThickness) {
			Proxy.Thickness(InThickness);
		}

		~ScopedThickness() { Proxy.Thickness(OldThickness); }
	};

	ScopedThickness ThicknessTemp(float InThickness) { return ScopedThickness(*this, InThickness); }

	/** Draw a box */
	FLineBatchProxy& DrawBox(const FBox& Box, const FMatrix& TM);
	FLineBatchProxy& DrawBox(const AActor* Actor);

	/** Draw a box */
	FLineBatchProxy& DrawBox(const FVector& Center, const FVector& Box);
	FLineBatchProxy& DrawBox(const FVector& Center, const float Size);

	/** Draw a box */
	FLineBatchProxy& DrawBox(const FVector& Center, const FVector& Box, const FQuat& Rotation);

	/** Draw a box */
	FLineBatchProxy& DrawBox(const FBox& Box);

	FLineBatchProxy& DrawBoundingBox(const FBox& Box);

	FLineBatchProxy& DrawSphere(const FVector& Center, float Radius, int32 Segments);

	FLineBatchProxy& DrawCone(const FVector& Origin, const FVector& Direction, float Length, float AngleWidth, float AngleHeight, int32 NumSides);

	FLineBatchProxy& DrawCapsule(const FVector& Center, float HalfHeight, float Radius, const FQuat& Rotation);

	FLineBatchProxy& DrawCircle(FVector Center, float Radius, int32 Segments, bool bDrawAxis = true, FVector YAxis = FVector(0.f, 1.f, 0.f), FVector ZAxis = FVector(0.f, 0.f, 1.f));
	FLineBatchProxy& DrawCircle(const FMatrix& TransformMatrix, float Radius, int32 Segments, bool bDrawAxis = true);

	/** Draw an arrow */
	FLineBatchProxy& DrawDirectionalArrow(const FMatrix& ArrowToWorld, float Length, float ArrowSize);
	/** Draw an arrow */
	FLineBatchProxy& DrawDirectionalArrow(const FVector& LineStart, const FVector& LineEnd, float ArrowSize);

#pragma region "Traces"

	FLineBatchProxy& DrawDebugSweptSphere(FVector const& Start, FVector const& End, float Radius);
	FLineBatchProxy& DrawDebugSweptSphere(FVector const& Start, FVector const& End, float Radius, FLinearColor TraceColor);

	FLineBatchProxy& DrawDebugSweptBox(FVector const& Start, FVector const& End, FRotator const& Orientation, FVector const& HalfSize);
	FLineBatchProxy& DrawDebugSweptBox(FVector const& Start, FVector const& End, FRotator const& Orientation, FVector const& HalfSize, FLinearColor TraceColor);

	FLineBatchProxy& DrawLineTraceSingle(const FHitResult& HitResult, const FVector& Start, const FVector& End, FLinearColor TraceColor = FLinearColor::Red,
	                                     FLinearColor TraceHitColor = FLinearColor::Green);

	/** Util for drawing result of multi line trace  */
	FLineBatchProxy& DrawDebugLineTraceMulti(const FVector& Start, const FVector& End, bool bHit, const TArray<FHitResult>& OutHits, FLinearColor TraceColor = FLinearColor::Red,
	                                         FLinearColor TraceHitColor = FLinearColor::Green);

	FLineBatchProxy& DrawDebugBoxTraceSingle(const FVector& Start, const FVector& End, const FVector HalfSize, const FRotator Orientation, bool bHit, const FHitResult& OutHit,
	                                         FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green);

	FLineBatchProxy& DrawDebugBoxTraceMulti(const FVector& Start, const FVector& End, const FVector HalfSize, const FRotator Orientation, bool bHit, const TArray<FHitResult>& OutHits,
	                                        FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green);

	FLineBatchProxy& DrawDebugSphereTraceSingle(const FVector& Start, const FVector& End, float Radius, bool bHit, const FHitResult& OutHit, FLinearColor TraceColor = FLinearColor::Red,
	                                            FLinearColor TraceHitColor = FLinearColor::Green);

	FLineBatchProxy& DrawDebugSphereTraceMulti(const FVector& Start, const FVector& End, float Radius, bool bHit, const TArray<FHitResult>& OutHits, FLinearColor TraceColor = FLinearColor::Red,
	                                           FLinearColor TraceHitColor = FLinearColor::Green);

	FLineBatchProxy& DrawDebugCapsuleTraceSingle(const FVector& Start, const FVector& End, float Radius, float HalfHeight, bool bHit, const FHitResult& OutHit,
	                                             FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green);

	FLineBatchProxy& DrawDebugCapsuleTraceMulti(const FVector& Start, const FVector& End, float Radius, float HalfHeight, bool bHit, const TArray<FHitResult>& OutHits,
	                                            FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green);

	FLineBatchProxy& DrawSolidQuad(const FBox& Box, const FTransform& Xform);
	FLineBatchProxy& DrawSolidQuad(const FBox& Box);

	FLineBatchProxy& DrawSolidBox(const FBox& Box);

#pragma endregion

	FLineBatchProxy& DrawLine(const FVector& Start, const FVector& End);
	FLineBatchProxy& DrawPoint(const FVector& Position, float PointSize);

	FLineBatchProxy& DrawLines(TArrayView<struct FBatchedLine> InLines);
	FLineBatchProxy& DrawLines(TArrayView<struct FPartialLine> InLines, uint32 InBatchID = INVALID_ID);

	virtual void QueueLineDraw(const FVector& Start, const FVector& End);
	virtual void QueueLineDraw(const FBatchedLine& Line);
	virtual void QueueLineDraw(TArrayView<struct FBatchedLine> InLines);

private:
	FLineBatchProxy& AddHalfCircle(TArray<FBatchedLine>& LineQueue, const FVector& Base, const FVector& X, const FVector& Y, const float Radius, int32 NumSides);
	FLineBatchProxy& AddCircle(TArray<FBatchedLine>& LineQueue, const FVector& Base, const FVector& X, const FVector& Y, const float Radius, int32 NumSides);
};
