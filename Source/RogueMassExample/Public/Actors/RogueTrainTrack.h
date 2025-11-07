// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RogueTrainTrack.generated.h"

class USplineMeshComponent;
class USplineComponent;

UCLASS()
class ROGUEMASSEXAMPLE_API ARogueTrainTrack : public AActor
{
	GENERATED_BODY()

public:
	ARogueTrainTrack();
	void BuildTrackMeshes();

protected:	
	void ClearTrackMeshes();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Components")
	USplineComponent* SplineComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Track")
	UStaticMesh* TrainTrackMesh = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Track")
	UMaterialInterface* TrackMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Track", meta=(ClampMin="0.01"))
	float TrainTrackScale = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Track", meta=(ClampMin="0.01"))
	float MeshLength = 200.f;

	UPROPERTY(EditAnywhere, Category="Track")
	float MeshOverlap = 8.f;  

	UPROPERTY(Transient)
	TArray<USplineMeshComponent*> TrackSegments;
};
