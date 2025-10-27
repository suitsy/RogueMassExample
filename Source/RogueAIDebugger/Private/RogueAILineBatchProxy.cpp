// Fill out your copyright notice in the Description page of Project Settings.


#include "RogueAILineBatchProxy.h"
#include "Components/LineBatchComponent.h"
#include "Engine/World.h"

FLineBatchProxy::FLineBatchProxy(float DefaultThickness): CurrentThickness(DefaultThickness) {}

FLineBatchProxy::FLineBatchProxy(const UWorld* World, float DefaultThickness) {
	CurrentThickness = DefaultThickness;
	if (World)
		LineBatcher = World->GetLineBatcher(UWorld::ELineBatcherType::Foreground);
}

FLineBatchProxy::FLineBatchProxy(ULineBatchComponent* InLineBatcher, float DefaultThickness) {
	CurrentThickness = DefaultThickness;
	LineBatcher = InLineBatcher;
}

FLineBatchProxy& FLineBatchProxy::Persistent(float InLifeTime) {
	LifeTime = InLifeTime;
	if (LineBatcher) {
		LineBatcher = LineBatcher->GetWorld()->GetLineBatcher(UWorld::ELineBatcherType::WorldPersistent);
	}
	return *this;
}

FLineBatchProxy& FLineBatchProxy::Foreground() {
	LifeTime = 0.0f;
	if (LineBatcher) {
		LineBatcher = LineBatcher->GetWorld()->GetLineBatcher(UWorld::ELineBatcherType::Foreground);
	}
	return *this;
}

FLineBatchProxy& FLineBatchProxy::Color(const FLinearColor& InColor) {
	CurrentColor = InColor;
	return *this;
}

FLineBatchProxy& FLineBatchProxy::Thickness(float InThickness) {
	CurrentThickness = InThickness;
	return *this;
}


FLineBatchProxy& FLineBatchProxy::DrawBox(const FBox& Box, const FMatrix& TM) {
	FVector B[2], P, Q;
	B[0] = Box.Min;
	B[1] = Box.Max;

	TArray<FBatchedLine, TInlineAllocator<12>> Lines;

	for (int32 ai = 0; ai < 2; ai++)
		for (int32 aj = 0; aj < 2; aj++) {
			P.X = B[ai].X;
			Q.X = B[ai].X;
			P.Y = B[aj].Y;
			Q.Y = B[aj].Y;
			P.Z = B[0].Z;
			Q.Z = B[1].Z;
			Lines.Add(FBatchedLine(TM.TransformPosition(P), TM.TransformPosition(Q), CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID));

			P.Y = B[ai].Y;
			Q.Y = B[ai].Y;
			P.Z = B[aj].Z;
			Q.Z = B[aj].Z;
			P.X = B[0].X;
			Q.X = B[1].X;
			Lines.Add(FBatchedLine(TM.TransformPosition(P), TM.TransformPosition(Q), CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID));

			P.Z = B[ai].Z;
			Q.Z = B[ai].Z;
			P.X = B[aj].X;
			Q.X = B[aj].X;
			P.Y = B[0].Y;
			Q.Y = B[1].Y;
			Lines.Add(FBatchedLine(TM.TransformPosition(P), TM.TransformPosition(Q), CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID));
		}

	QueueLineDraw(Lines);

	return *this;
}

FLineBatchProxy& FLineBatchProxy::DrawBox(const AActor* Actor) {
	if (!Actor) return *this;
	const FBox Box = Actor->GetComponentsBoundingBox(false, true);
	return DrawBox(Box.GetCenter(), Box.GetExtent(), Actor->GetActorRotation().Quaternion());
}

FLineBatchProxy& FLineBatchProxy::DrawBox(const FVector& Center, const FVector& Box) {
	TArray<FBatchedLine, TInlineAllocator<12>> BatchedLines;
	BatchedLines.Emplace(Center + FVector(Box.X, Box.Y, Box.Z), Center + FVector(Box.X, -Box.Y, Box.Z), CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID);
	BatchedLines.Emplace(Center + FVector(Box.X, -Box.Y, Box.Z), Center + FVector(-Box.X, -Box.Y, Box.Z), CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID);
	BatchedLines.Emplace(Center + FVector(-Box.X, -Box.Y, Box.Z), Center + FVector(-Box.X, Box.Y, Box.Z), CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID);
	BatchedLines.Emplace(Center + FVector(-Box.X, Box.Y, Box.Z), Center + FVector(Box.X, Box.Y, Box.Z), CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID);

	BatchedLines.Emplace(Center + FVector(Box.X, Box.Y, -Box.Z), Center + FVector(Box.X, -Box.Y, -Box.Z), CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID);
	BatchedLines.Emplace(Center + FVector(Box.X, -Box.Y, -Box.Z), Center + FVector(-Box.X, -Box.Y, -Box.Z), CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID);
	BatchedLines.Emplace(Center + FVector(-Box.X, -Box.Y, -Box.Z), Center + FVector(-Box.X, Box.Y, -Box.Z), CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID);
	BatchedLines.Emplace(Center + FVector(-Box.X, Box.Y, -Box.Z), Center + FVector(Box.X, Box.Y, -Box.Z), CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID);

	BatchedLines.Emplace(Center + FVector(Box.X, Box.Y, Box.Z), Center + FVector(Box.X, Box.Y, -Box.Z), CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID);
	BatchedLines.Emplace(Center + FVector(Box.X, -Box.Y, Box.Z), Center + FVector(Box.X, -Box.Y, -Box.Z), CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID);
	BatchedLines.Emplace(Center + FVector(-Box.X, -Box.Y, Box.Z), Center + FVector(-Box.X, -Box.Y, -Box.Z), CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID);
	BatchedLines.Emplace(Center + FVector(-Box.X, Box.Y, Box.Z), Center + FVector(-Box.X, Box.Y, -Box.Z), CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID);

	QueueLineDraw(BatchedLines);

	return *this;
}

FLineBatchProxy& FLineBatchProxy::DrawBox(const FVector& Center, const float Size) {
	const FBox Box = FBox::BuildAABB(Center, FVector(Size));
	return DrawBox(Box);
}

FLineBatchProxy& FLineBatchProxy::DrawBox(const FVector& Center, const FVector& Box, const FQuat& Rotation) {
	TArray<FBatchedLine, TInlineAllocator<12>> BatchedLines;

	FTransform const Transform(Rotation);

	FVector Start = Transform.TransformPosition(FVector(Box.X, Box.Y, Box.Z));
	FVector End = Transform.TransformPosition(FVector(Box.X, -Box.Y, Box.Z));
	BatchedLines.Emplace(Center + Start, Center + End, CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID);

	Start = Transform.TransformPosition(FVector(Box.X, -Box.Y, Box.Z));
	End = Transform.TransformPosition(FVector(-Box.X, -Box.Y, Box.Z));
	BatchedLines.Emplace(Center + Start, Center + End, CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID);

	Start = Transform.TransformPosition(FVector(-Box.X, -Box.Y, Box.Z));
	End = Transform.TransformPosition(FVector(-Box.X, Box.Y, Box.Z));
	BatchedLines.Emplace(Center + Start, Center + End, CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID);

	Start = Transform.TransformPosition(FVector(-Box.X, Box.Y, Box.Z));
	End = Transform.TransformPosition(FVector(Box.X, Box.Y, Box.Z));
	BatchedLines.Emplace(Center + Start, Center + End, CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID);

	Start = Transform.TransformPosition(FVector(Box.X, Box.Y, -Box.Z));
	End = Transform.TransformPosition(FVector(Box.X, -Box.Y, -Box.Z));
	BatchedLines.Emplace(Center + Start, Center + End, CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID);

	Start = Transform.TransformPosition(FVector(Box.X, -Box.Y, -Box.Z));
	End = Transform.TransformPosition(FVector(-Box.X, -Box.Y, -Box.Z));
	BatchedLines.Emplace(Center + Start, Center + End, CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID);

	Start = Transform.TransformPosition(FVector(-Box.X, -Box.Y, -Box.Z));
	End = Transform.TransformPosition(FVector(-Box.X, Box.Y, -Box.Z));
	BatchedLines.Emplace(Center + Start, Center + End, CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID);

	Start = Transform.TransformPosition(FVector(-Box.X, Box.Y, -Box.Z));
	End = Transform.TransformPosition(FVector(Box.X, Box.Y, -Box.Z));
	BatchedLines.Emplace(Center + Start, Center + End, CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID);

	Start = Transform.TransformPosition(FVector(Box.X, Box.Y, Box.Z));
	End = Transform.TransformPosition(FVector(Box.X, Box.Y, -Box.Z));
	BatchedLines.Emplace(Center + Start, Center + End, CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID);

	Start = Transform.TransformPosition(FVector(Box.X, -Box.Y, Box.Z));
	End = Transform.TransformPosition(FVector(Box.X, -Box.Y, -Box.Z));
	BatchedLines.Emplace(Center + Start, Center + End, CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID);

	Start = Transform.TransformPosition(FVector(-Box.X, -Box.Y, Box.Z));
	End = Transform.TransformPosition(FVector(-Box.X, -Box.Y, -Box.Z));
	BatchedLines.Emplace(Center + Start, Center + End, CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID);

	Start = Transform.TransformPosition(FVector(-Box.X, Box.Y, Box.Z));
	End = Transform.TransformPosition(FVector(-Box.X, Box.Y, -Box.Z));
	BatchedLines.Emplace(Center + Start, Center + End, CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID);

	QueueLineDraw(BatchedLines);

	return *this;
}

FLineBatchProxy& FLineBatchProxy::DrawBox(const FBox& Box) {
	const auto Min = Box.Min;
	const auto Max = Box.Max;

	TArray<FBatchedLine, TInlineAllocator<4>> Lines;
	Lines.SetNumUninitialized(4);
	Lines[0] = FBatchedLine(Min, FVector(Max.X, Min.Y, Min.Z), CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID);
	Lines[1] = FBatchedLine(FVector(Max.X, Min.Y, Min.Z), Max, CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID);
	Lines[2] = FBatchedLine(Max, FVector(Min.X, Max.Y, Min.Z), CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID);
	Lines[3] = FBatchedLine(FVector(Min.X, Max.Y, Min.Z), Min, CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID);

	return DrawLines(Lines);
}

FLineBatchProxy& FLineBatchProxy::DrawBoundingBox(const FBox& Box) {
	return DrawBox(Box.GetCenter(), Box.GetExtent());
}

FLineBatchProxy& FLineBatchProxy::DrawSphere(const FVector& Center, float Radius, int32 Segments) {
	// Need at least 4 segments
	Segments = FMath::Max(Segments, 4);

	const float AngleInc = 2.f * UE_PI / Segments;
	int32 NumSegmentsY = Segments;
	float Latitude = AngleInc;
	float SinY1 = 0.0f;
	float CosY1 = 1.0f;

	TArray<FBatchedLine> Lines;
	Lines.Empty(NumSegmentsY * Segments * 2);

	while (NumSegmentsY--) {
		const float SinY2 = FMath::Sin(Latitude);
		const float CosY2 = FMath::Cos(Latitude);

		FVector Vertex1 = FVector(SinY1, 0.0f, CosY1) * Radius + Center;
		FVector Vertex3 = FVector(SinY2, 0.0f, CosY2) * Radius + Center;
		float Longitude = AngleInc;

		int32 NumSegmentsX = Segments;
		while (NumSegmentsX--) {
			const float SinX = FMath::Sin(Longitude);
			const float CosX = FMath::Cos(Longitude);

			const FVector Vertex2 = FVector((CosX * SinY1), (SinX * SinY1), CosY1) * Radius + Center;
			const FVector Vertex4 = FVector((CosX * SinY2), (SinX * SinY2), CosY2) * Radius + Center;

			Lines.Emplace(Vertex1, Vertex2, CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID);
			Lines.Emplace(Vertex1, Vertex3, CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID);

			Vertex1 = Vertex2;
			Vertex3 = Vertex4;
			Longitude += AngleInc;
		}

		SinY1 = SinY2;
		CosY1 = CosY2;
		Latitude += AngleInc;
	}

	QueueLineDraw(Lines);

	return *this;
}

FLineBatchProxy& FLineBatchProxy::DrawCone(const FVector& Origin, const FVector& Direction, float Length, float AngleWidth, float AngleHeight, int32 NumSides) {
	TArray<FBatchedLine> Lines;

	NumSides = FMath::Max(NumSides, 4);

	const float Angle1 = FMath::Clamp<float>(AngleHeight, UE_KINDA_SMALL_NUMBER, (UE_PI - UE_KINDA_SMALL_NUMBER));
	const float Angle2 = FMath::Clamp<float>(AngleWidth, UE_KINDA_SMALL_NUMBER, (UE_PI - UE_KINDA_SMALL_NUMBER));

	const float SinX_2 = FMath::Sin(0.5f * Angle1);
	const float SinY_2 = FMath::Sin(0.5f * Angle2);

	const float SinSqX_2 = SinX_2 * SinX_2;
	const float SinSqY_2 = SinY_2 * SinY_2;

	TArray<FVector> ConeVerts;
	ConeVerts.AddUninitialized(NumSides);

	for (int32 i = 0; i < NumSides; i++) {
		const float Fraction = (float)i / (float)(NumSides);
		const float Thi = 2.f * UE_PI * Fraction;
		const float Phi = FMath::Atan2(FMath::Sin(Thi) * SinY_2, FMath::Cos(Thi) * SinX_2);
		const float SinPhi = FMath::Sin(Phi);
		const float CosPhi = FMath::Cos(Phi);
		const float SinSqPhi = SinPhi * SinPhi;
		const float CosSqPhi = CosPhi * CosPhi;

		const float RSq = SinSqX_2 * SinSqY_2 / (SinSqX_2 * SinSqPhi + SinSqY_2 * CosSqPhi);
		const float R = FMath::Sqrt(RSq);
		const float Sqr = FMath::Sqrt(1 - RSq);
		const float Alpha = R * CosPhi;
		const float Beta = R * SinPhi;

		ConeVerts[i].X = (1 - 2 * RSq);
		ConeVerts[i].Y = 2 * Sqr * Alpha;
		ConeVerts[i].Z = 2 * Sqr * Beta;
	}

	// Calculate transform for cone.
	FVector YAxis, ZAxis;
	const FVector DirectionNorm = Direction.GetSafeNormal();
	DirectionNorm.FindBestAxisVectors(YAxis, ZAxis);
	const FMatrix ConeToWorld = FScaleMatrix(FVector(Length)) * FMatrix(DirectionNorm, YAxis, ZAxis, Origin);

	FVector CurrentPoint, PrevPoint, FirstPoint;
	for (int32 i = 0; i < NumSides; i++) {
		CurrentPoint = ConeToWorld.TransformPosition(ConeVerts[i]);
		Lines.Emplace(ConeToWorld.GetOrigin(), CurrentPoint, CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID);

		// PrevPoint must be defined to draw junctions
		if (i > 0) {
			Lines.Emplace(PrevPoint, CurrentPoint, CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID);
		} else {
			FirstPoint = CurrentPoint;
		}

		PrevPoint = CurrentPoint;
	}
	// Connect last junction to first
	Lines.Emplace(CurrentPoint, FirstPoint, CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID);

	QueueLineDraw(Lines);

	return *this;
}

FLineBatchProxy& FLineBatchProxy::DrawCapsule(const FVector& Center, float HalfHeight, float Radius, const FQuat& Rotation) {
	TArray<FBatchedLine> Lines;

	constexpr int32 DrawCollisionSides = 16;
	const FVector Origin = Center;
	const FMatrix Axes = FQuatRotationTranslationMatrix(Rotation, FVector::ZeroVector);
	const FVector XAxis = Axes.GetScaledAxis(EAxis::X);
	const FVector YAxis = Axes.GetScaledAxis(EAxis::Y);
	const FVector ZAxis = Axes.GetScaledAxis(EAxis::Z);

	// Draw top and bottom circles
	const float HalfAxis = FMath::Max<float>(HalfHeight - Radius, 1.f);
	const FVector TopEnd = Origin + HalfAxis * ZAxis;
	const FVector BottomEnd = Origin - HalfAxis * ZAxis;

	AddCircle(Lines, TopEnd, XAxis, YAxis, Radius, DrawCollisionSides);
	AddCircle(Lines, BottomEnd, XAxis, YAxis, Radius, DrawCollisionSides);

	// Draw domed caps
	AddHalfCircle(Lines, TopEnd, YAxis, ZAxis, Radius, DrawCollisionSides);
	AddHalfCircle(Lines, TopEnd, XAxis, ZAxis, Radius, DrawCollisionSides);

	const FVector NegZAxis = -ZAxis;

	AddHalfCircle(Lines, BottomEnd, YAxis, NegZAxis, Radius, DrawCollisionSides);
	AddHalfCircle(Lines, BottomEnd, XAxis, NegZAxis, Radius, DrawCollisionSides);

	// Draw connected lines
	Lines.Emplace(TopEnd + Radius * XAxis, BottomEnd + Radius * XAxis, CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID);
	Lines.Emplace(TopEnd - Radius * XAxis, BottomEnd - Radius * XAxis, CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID);
	Lines.Emplace(TopEnd + Radius * XAxis, BottomEnd + Radius * XAxis, CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID);
	Lines.Emplace(TopEnd - Radius * XAxis, BottomEnd - Radius * XAxis, CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID);

	QueueLineDraw(Lines);

	return *this;
}

FLineBatchProxy& FLineBatchProxy::DrawCircle(FVector Center, float Radius, int32 Segments, bool bDrawAxis, FVector YAxis, FVector ZAxis) {
	FMatrix TM;
	TM.SetOrigin(Center);
	TM.SetAxis(0, FVector(1, 0, 0));
	TM.SetAxis(1, YAxis);
	TM.SetAxis(2, ZAxis);

	DrawCircle(TM, Radius, Segments, bDrawAxis);

	return *this;
}

FLineBatchProxy& FLineBatchProxy::DrawCircle(const FMatrix& TransformMatrix, float Radius, int32 Segments, bool bDrawAxis) {
	Segments = FMath::Max(Segments, 4);

	const float AngleStep = 2.f * UE_PI / static_cast<float>(Segments);

	const FVector Center = TransformMatrix.GetOrigin();
	const FVector AxisY = TransformMatrix.GetScaledAxis(EAxis::Y);
	const FVector AxisZ = TransformMatrix.GetScaledAxis(EAxis::Z);

	TArray<FBatchedLine> Lines;
	Lines.Empty(Segments);

	float Angle = 0.f;
	while (Segments--) {
		const FVector Vertex1 = Center + Radius * (AxisY * FMath::Cos(Angle) + AxisZ * FMath::Sin(Angle));
		Angle += AngleStep;
		const FVector Vertex2 = Center + Radius * (AxisY * FMath::Cos(Angle) + AxisZ * FMath::Sin(Angle));
		Lines.Add(FBatchedLine(Vertex1, Vertex2, CurrentColor, LifeTime, CurrentThickness, DepthPriority));
	}

	if (bDrawAxis) {
		Lines.Add(FBatchedLine(Center - Radius * AxisY, Center + Radius * AxisY, CurrentColor, LifeTime, CurrentThickness, DepthPriority));
		Lines.Add(FBatchedLine(Center - Radius * AxisZ, Center + Radius * AxisZ, CurrentColor, LifeTime, CurrentThickness, DepthPriority));
	}

	QueueLineDraw(Lines);

	return *this;
}

FLineBatchProxy& FLineBatchProxy::DrawDirectionalArrow(const FMatrix& ArrowToWorld, float Length, float ArrowSize) {
	const FVector Tip = ArrowToWorld.TransformPosition(FVector(Length, 0, 0));

	TArray<FBatchedLine, TInlineAllocator<5>> BatchedLines;

	BatchedLines.Emplace(Tip, ArrowToWorld.TransformPosition(FVector::ZeroVector), CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID);
	BatchedLines.Emplace(Tip, ArrowToWorld.TransformPosition(FVector(Length - ArrowSize, +ArrowSize, +ArrowSize)), CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID);
	BatchedLines.Emplace(Tip, ArrowToWorld.TransformPosition(FVector(Length - ArrowSize, +ArrowSize, -ArrowSize)), CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID);
	BatchedLines.Emplace(Tip, ArrowToWorld.TransformPosition(FVector(Length - ArrowSize, -ArrowSize, +ArrowSize)), CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID);
	BatchedLines.Emplace(Tip, ArrowToWorld.TransformPosition(FVector(Length - ArrowSize, -ArrowSize, -ArrowSize)), CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID);

	QueueLineDraw(BatchedLines);

	return *this;
}

FLineBatchProxy& FLineBatchProxy::DrawDirectionalArrow(const FVector& LineStart, const FVector& LineEnd, float ArrowSize) {
	TArray<FBatchedLine, TInlineAllocator<3>> BatchedLines;

	FVector Dir = (LineEnd - LineStart);
	Dir.Normalize();
	FVector Up(0, 0, 1);
	FVector Right = Dir ^ Up;
	if (!Right.IsNormalized()) {
		Dir.FindBestAxisVectors(Up, Right);
	}
	const FVector Origin = FVector::ZeroVector;
	FMatrix TM;
	// get matrix with dir/right/up
	TM.SetAxes(&Dir, &Right, &Up, &Origin);

	// since dir is x direction, my arrow will be pointing +y, -x and -y, -x
	const float ArrowSqrt = FMath::Sqrt(ArrowSize);

	BatchedLines.Emplace(LineStart, LineEnd, CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID);
	BatchedLines.Emplace(LineEnd, LineEnd + TM.TransformPosition(FVector(-ArrowSqrt, ArrowSqrt, 0)), CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID);
	BatchedLines.Emplace(LineEnd, LineEnd + TM.TransformPosition(FVector(-ArrowSqrt, -ArrowSqrt, 0)), CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID);

	QueueLineDraw(BatchedLines);

	return *this;
}

#pragma region "Traces"

FLineBatchProxy& FLineBatchProxy::DrawDebugSweptSphere(FVector const& Start, FVector const& End, float Radius) {
	FVector const TraceVec = End - Start;
	float const Dist = TraceVec.Size();

	FVector const Center = Start + TraceVec * 0.5f;
	float const HalfHeight = (Dist * 0.5f) + Radius;

	FQuat const CapsuleRot = FRotationMatrix::MakeFromZ(TraceVec).ToQuat();

	return DrawCapsule(Center, HalfHeight, Radius, CapsuleRot);
}

FLineBatchProxy& FLineBatchProxy::DrawDebugSweptSphere(FVector const& Start, FVector const& End, float Radius, FLinearColor TraceColor) {
	auto ScopedColor = ColorTemp(TraceColor);
	return DrawDebugSweptSphere(Start, End, Radius);
}

FLineBatchProxy& FLineBatchProxy::DrawDebugSweptBox(FVector const& Start, FVector const& End, FRotator const& Orientation, FVector const& HalfSize) {
	FVector const TraceVec = End - Start;

	FQuat const CapsuleRot = Orientation.Quaternion();
	DrawBox(Start, HalfSize, CapsuleRot);

	//now draw lines from vertices
	FVector Vertices[8];
	Vertices[0] = Start + CapsuleRot.RotateVector(FVector(-HalfSize.X, -HalfSize.Y, -HalfSize.Z));
	Vertices[1] = Start + CapsuleRot.RotateVector(FVector(-HalfSize.X, HalfSize.Y, -HalfSize.Z));
	Vertices[2] = Start + CapsuleRot.RotateVector(FVector(-HalfSize.X, -HalfSize.Y, HalfSize.Z));
	Vertices[3] = Start + CapsuleRot.RotateVector(FVector(-HalfSize.X, HalfSize.Y, HalfSize.Z));
	Vertices[4] = Start + CapsuleRot.RotateVector(FVector(HalfSize.X, -HalfSize.Y, -HalfSize.Z));
	Vertices[5] = Start + CapsuleRot.RotateVector(FVector(HalfSize.X, HalfSize.Y, -HalfSize.Z));
	Vertices[6] = Start + CapsuleRot.RotateVector(FVector(HalfSize.X, -HalfSize.Y, HalfSize.Z));
	Vertices[7] = Start + CapsuleRot.RotateVector(FVector(HalfSize.X, HalfSize.Y, HalfSize.Z));
	for (int32 VertexIdx = 0; VertexIdx < 8; ++VertexIdx) {
		DrawLine(Vertices[VertexIdx], Vertices[VertexIdx] + TraceVec);
	}

	DrawBox(End, HalfSize, CapsuleRot);

	return *this;
}

FLineBatchProxy& FLineBatchProxy::DrawDebugSweptBox(FVector const& Start, FVector const& End, FRotator const& Orientation, FVector const& HalfSize, FLinearColor TraceColor) {
	auto ScopedColor = ColorTemp(TraceColor);
	return DrawDebugSweptBox(Start, End, Orientation, HalfSize);
}

FLineBatchProxy& FLineBatchProxy::DrawLineTraceSingle(const FHitResult& HitResult, const FVector& Start, const FVector& End, FLinearColor TraceColor, FLinearColor TraceHitColor) {
	const bool bHit = HitResult.bBlockingHit;
	if (bHit) {
		TArray<FBatchedLine, TInlineAllocator<2>> Lines;
		Lines.SetNumUninitialized(2);
		Lines[0] = FBatchedLine(Start, HitResult.ImpactPoint, TraceColor, LifeTime, CurrentThickness, DepthPriority);
		Lines[1] = FBatchedLine(HitResult.ImpactPoint, End, TraceHitColor, LifeTime, CurrentThickness, DepthPriority);
		DrawLines(Lines);
		{
			auto ScopedColor = ColorTemp(TraceColor);
			DrawSphere(HitResult.ImpactPoint, 10.0f, 8);
		}
	} else {
		DrawLine(Start, End);
	}
	return *this;
}

FLineBatchProxy& FLineBatchProxy::DrawDebugLineTraceMulti(const FVector& Start, const FVector& End, bool bHit, const TArray<FHitResult>& OutHits, FLinearColor TraceColor, FLinearColor TraceHitColor) {
	if (bHit && OutHits.Last().bBlockingHit) {
		FVector const BlockingHitPoint = OutHits.Last().ImpactPoint;
		{
			auto ScopedColor = ColorTemp(TraceColor.ToFColor(true));
			QueueLineDraw(Start, BlockingHitPoint);
		}
		{
			auto ScopedColor = ColorTemp(TraceHitColor.ToFColor(true));
			QueueLineDraw(BlockingHitPoint, End);
		}
	} else {
		auto ScopedColor = ColorTemp(TraceColor.ToFColor(true));
		QueueLineDraw(Start, End);
	}

	for (int32 HitIdx = 0; HitIdx < OutHits.Num(); ++HitIdx) {
		FHitResult const& Hit = OutHits[HitIdx];
		auto ScopedColor = ColorTemp((Hit.bBlockingHit ? TraceColor.ToFColor(true) : TraceHitColor.ToFColor(true)));
		DrawPoint(Hit.ImpactPoint, 16.f);
	}

	return *this;
}

FLineBatchProxy& FLineBatchProxy::DrawDebugBoxTraceSingle(const FVector& Start, const FVector& End, const FVector HalfSize, const FRotator Orientation, bool bHit, const FHitResult& OutHit,
                                                          FLinearColor TraceColor, FLinearColor TraceHitColor) {
	if (bHit && OutHit.bBlockingHit) {
		DrawDebugSweptBox(Start, OutHit.Location, Orientation, HalfSize, TraceColor.ToFColor(true));
		DrawDebugSweptBox(OutHit.Location, End, Orientation, HalfSize, TraceHitColor.ToFColor(true));
		{
			auto ScopedColor = ColorTemp(TraceColor.ToFColor(true));
			DrawPoint(OutHit.ImpactPoint, 16.0f);
		}
	} else {
		DrawDebugSweptBox(Start, End, Orientation, HalfSize, TraceColor.ToFColor(true));
	}
	return *this;
}

FLineBatchProxy& FLineBatchProxy::DrawDebugBoxTraceMulti(const FVector& Start, const FVector& End, const FVector HalfSize, const FRotator Orientation, bool bHit, const TArray<FHitResult>& OutHits,
                                                         FLinearColor TraceColor, FLinearColor TraceHitColor) {
	if (bHit && OutHits.Last().bBlockingHit) {
		FVector const BlockingHitPoint = OutHits.Last().Location;
		DrawDebugSweptBox(Start, BlockingHitPoint, Orientation, HalfSize, TraceColor.ToFColor(true));
		DrawDebugSweptBox(BlockingHitPoint, End, Orientation, HalfSize, TraceHitColor.ToFColor(true));
	} else {
		DrawDebugSweptBox(Start, End, Orientation, HalfSize, TraceColor.ToFColor(true));
	}

	// draw hits
	for (int32 HitIdx = 0; HitIdx < OutHits.Num(); ++HitIdx) {
		FHitResult const& Hit = OutHits[HitIdx];
		auto ScopedColor = ColorTemp((Hit.bBlockingHit ? TraceColor.ToFColor(true) : TraceHitColor.ToFColor(true)));
		DrawPoint(Hit.ImpactPoint, 16.f);
	}


	return *this;
}

FLineBatchProxy& FLineBatchProxy::DrawDebugSphereTraceSingle(const FVector& Start, const FVector& End, float Radius, bool bHit, const FHitResult& OutHit, FLinearColor TraceColor,
                                                             FLinearColor TraceHitColor) {
	if (bHit && OutHit.bBlockingHit) {
		DrawDebugSweptSphere(Start, OutHit.Location, Radius, TraceColor.ToFColor(true));
		DrawDebugSweptSphere(OutHit.Location, End, Radius, TraceHitColor.ToFColor(true));
		{
			auto ScopedColor = ColorTemp(TraceColor.ToFColor(true));
			DrawPoint(OutHit.ImpactPoint, 16.0f);
		}
	} else {
		DrawDebugSweptSphere(Start, End, Radius, TraceColor.ToFColor(true));
	}


	return *this;
}

FLineBatchProxy& FLineBatchProxy::DrawDebugSphereTraceMulti(const FVector& Start, const FVector& End, float Radius, bool bHit, const TArray<FHitResult>& OutHits, FLinearColor TraceColor,
                                                            FLinearColor TraceHitColor) {
	if (bHit && OutHits.Last().bBlockingHit) {
		FVector const BlockingHitPoint = OutHits.Last().Location;
		DrawDebugSweptSphere(Start, BlockingHitPoint, Radius, TraceColor.ToFColor(true));
		DrawDebugSweptSphere(BlockingHitPoint, End, Radius, TraceHitColor.ToFColor(true));
	} else {
		DrawDebugSweptSphere(Start, End, Radius, TraceColor.ToFColor(true));
	}

	// draw hits
	for (int32 HitIdx = 0; HitIdx < OutHits.Num(); ++HitIdx) {
		FHitResult const& Hit = OutHits[HitIdx];

		auto ScopedColor = ColorTemp((Hit.bBlockingHit ? TraceColor.ToFColor(true) : TraceHitColor.ToFColor(true)));
		DrawPoint(Hit.ImpactPoint, 16.f);
	}


	return *this;
}

FLineBatchProxy& FLineBatchProxy::DrawDebugCapsuleTraceSingle(const FVector& Start, const FVector& End, float Radius, float HalfHeight, bool bHit, const FHitResult& OutHit, FLinearColor TraceColor,
                                                              FLinearColor TraceHitColor) {
	if (bHit && OutHit.bBlockingHit) {
		{
			auto ScopedColor = ColorTemp(TraceColor.ToFColor(true));
			DrawCapsule(Start, HalfHeight, Radius, FQuat::Identity);
			DrawCapsule(OutHit.Location, HalfHeight, Radius, FQuat::Identity);
			DrawLine(Start, OutHit.Location);
			DrawPoint(OutHit.ImpactPoint, 16.0f);
		}
		{
			auto ScopedColor = ColorTemp(TraceHitColor.ToFColor(true));
			DrawCapsule(End, HalfHeight, Radius, FQuat::Identity);
			DrawLine(OutHit.Location, End);
		}
	} else {
		auto ScopedColor = ColorTemp(TraceColor.ToFColor(true));
		DrawCapsule(Start, HalfHeight, Radius, FQuat::Identity);
		DrawCapsule(End, HalfHeight, Radius, FQuat::Identity);
		DrawLine(Start, End);
	}


	return *this;
}

FLineBatchProxy& FLineBatchProxy::DrawDebugCapsuleTraceMulti(const FVector& Start, const FVector& End, float Radius, float HalfHeight, bool bHit, const TArray<FHitResult>& OutHits,
                                                             FLinearColor TraceColor, FLinearColor TraceHitColor) {
	if (bHit && OutHits.Last().bBlockingHit) {
		FVector const BlockingHitPoint = OutHits.Last().Location;
		{
			auto ScopedColor = ColorTemp(TraceColor.ToFColor(true));
			DrawCapsule(Start, HalfHeight, Radius, FQuat::Identity);
			DrawCapsule(BlockingHitPoint, HalfHeight, Radius, FQuat::Identity);
			DrawLine(Start, BlockingHitPoint);
		}
		{
			auto ScopedColor = ColorTemp(TraceHitColor.ToFColor(true));
			DrawCapsule(End, HalfHeight, Radius, FQuat::Identity);
			DrawLine(BlockingHitPoint, End);
		}
	} else {
		auto ScopedColor = ColorTemp(TraceColor.ToFColor(true));
		DrawCapsule(Start, HalfHeight, Radius, FQuat::Identity);
		DrawCapsule(End, HalfHeight, Radius, FQuat::Identity);
		DrawLine(Start, End);
	}

	for (int32 HitIdx = 0; HitIdx < OutHits.Num(); ++HitIdx) {
		FHitResult const& Hit = OutHits[HitIdx];

		auto ScopedColor = ColorTemp((Hit.bBlockingHit ? TraceColor.ToFColor(true) : TraceHitColor.ToFColor(true)));
		DrawPoint(Hit.ImpactPoint, 16.f);
	}


	return *this;
}


#pragma endregion

FLineBatchProxy& FLineBatchProxy::DrawSolidQuad(const FBox& Box, const FTransform& Xform) {
	if (!LineBatcher) return *this;

	TArray<FVector> MeshVerts = {
		Xform.TransformPosition(FVector(Box.Min.X, Box.Min.Y, Box.Min.Z)),
		Xform.TransformPosition(FVector(Box.Max.X, Box.Min.Y, Box.Min.Z)),
		Xform.TransformPosition(FVector(Box.Min.X, Box.Max.Y, Box.Min.Z)),
		Xform.TransformPosition(FVector(Box.Max.X, Box.Max.Y, Box.Min.Z)),
	};
	TArray<int32> Indices = {0, 1, 2, 2, 1, 3};
	TArray<int32> MeshIndices;
	MeshIndices.SetNumUninitialized(6);

	for (int32 Idx = 0; Idx < 6; ++Idx) {
		MeshIndices[Idx] = Indices[Idx];
	}

	LineBatcher->DrawMesh(MeshVerts, MeshIndices, CurrentColor.ToFColor(true), DepthPriority, LifeTime, BatchID);

	return *this;
}

FLineBatchProxy& FLineBatchProxy::DrawSolidQuad(const FBox& Box) {
	if (!LineBatcher) return *this;

	TArray<FVector> MeshVerts = {
		FVector(Box.Min.X, Box.Min.Y, Box.Min.Z),
		FVector(Box.Max.X, Box.Min.Y, Box.Min.Z),
		FVector(Box.Min.X, Box.Max.Y, Box.Min.Z),
		FVector(Box.Max.X, Box.Max.Y, Box.Min.Z),
	};
	TArray<int32> Indices = {0, 1, 2, 2, 1, 3};
	TArray<int32> MeshIndices;
	MeshIndices.SetNumUninitialized(6);

	for (int32 Idx = 0; Idx < 6; ++Idx) {
		MeshIndices[Idx] = Indices[Idx];
	}

	LineBatcher->DrawMesh(MeshVerts, MeshIndices, CurrentColor.ToFColor(true), DepthPriority, LifeTime, BatchID);

	return *this;
}

FLineBatchProxy& FLineBatchProxy::DrawSolidBox(const FBox& Box) {
	if (!LineBatcher) return *this;

	LineBatcher->DrawSolidBox(Box, FTransform::Identity, CurrentColor.ToFColorSRGB(), DepthPriority, LifeTime, BatchID);

	return *this;
}


FLineBatchProxy& FLineBatchProxy::DrawLine(const FVector& Start, const FVector& End) {
	QueueLineDraw(Start, End);
	return *this;
}

FLineBatchProxy& FLineBatchProxy::DrawPoint(const FVector& Position, float PointSize) {
	if (!LineBatcher) return *this;
	LineBatcher->DrawPoint(Position, CurrentColor, PointSize, DepthPriority, LifeTime, BatchID);
	return *this;
}

FLineBatchProxy& FLineBatchProxy::DrawLines(TArrayView<FBatchedLine> InLines) {
	QueueLineDraw(InLines);
	return *this;
}

FLineBatchProxy& FLineBatchProxy::DrawLines(TArrayView<FPartialLine> InLines, uint32 InBatchID) {
	const uint32 BIDToUse = FMath::Max(InBatchID, BatchID);

	TArray<FBatchedLine> BatchedLines;
	BatchedLines.Reserve(InLines.Num());
	for (const FPartialLine& Line : InLines) {
		BatchedLines.Emplace(Line.Start, Line.End, CurrentColor, LifeTime, CurrentThickness, DepthPriority, BIDToUse);
	}

	QueueLineDraw(BatchedLines);

	return *this;
}

void FLineBatchProxy::QueueLineDraw(const FVector& Start, const FVector& End) {
	QueueLineDraw(FBatchedLine(Start, End, CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID));
}

void FLineBatchProxy::QueueLineDraw(const FBatchedLine& Line) {
	if (!LineBatcher) return;
	LineBatcher->DrawLine(Line.Start, Line.End, Line.Color, Line.DepthPriority, Line.Thickness, Line.RemainingLifeTime, Line.BatchID);
}

void FLineBatchProxy::QueueLineDraw(TArrayView<FBatchedLine> InLines) {
	if (!LineBatcher) return;
	LineBatcher->DrawLines(InLines);
}

FLineBatchProxy& FLineBatchProxy::AddHalfCircle(TArray<FBatchedLine>& LineQueue, const FVector& Base, const FVector& X, const FVector& Y, const float Radius, int32 NumSides) {
	// Need at least 2 sides
	NumSides = FMath::Max(NumSides, 2);
	const float AngleDelta = 2.0f * UE_PI / NumSides;
	FVector LastVertex = Base + X * Radius;

	for (int32 SideIndex = 0; SideIndex < (NumSides / 2); SideIndex++) {
		FVector Vertex = Base + (X * FMath::Cos(AngleDelta * (SideIndex + 1)) + Y * FMath::Sin(AngleDelta * (SideIndex + 1))) * Radius;
		LineQueue.Add(FBatchedLine(LastVertex, Vertex, CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID));
		LastVertex = Vertex;
	}

	return *this;
}

FLineBatchProxy& FLineBatchProxy::AddCircle(TArray<FBatchedLine>& LineQueue, const FVector& Base, const FVector& X, const FVector& Y, const float Radius, int32 NumSides) {
	// Need at least 2 sides
	NumSides = FMath::Max(NumSides, 2);
	const float AngleDelta = 2.0f * UE_PI / NumSides;
	FVector LastVertex = Base + X * Radius;

	for (int32 SideIndex = 0; SideIndex < NumSides; SideIndex++) {
		const FVector Vertex = Base + (X * FMath::Cos(AngleDelta * (SideIndex + 1)) + Y * FMath::Sin(AngleDelta * (SideIndex + 1))) * Radius;
		LineQueue.Add(FBatchedLine(LastVertex, Vertex, CurrentColor, LifeTime, CurrentThickness, DepthPriority, BatchID));
		LastVertex = Vertex;
	}

	return *this;
}