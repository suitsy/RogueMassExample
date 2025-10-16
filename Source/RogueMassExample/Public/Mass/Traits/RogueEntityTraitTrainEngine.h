// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityTraitBase.h"
#include "RogueEntityTraitTrainEngine.generated.h"

/**
 * 
 */
UCLASS(EditInlineNew, meta=(DisplayName="Train Engine Trait"))
class ROGUEMASSEXAMPLE_API URogueEntityTraitTrainEngine : public UMassEntityTraitBase
{
	GENERATED_BODY()

protected:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override;
};
