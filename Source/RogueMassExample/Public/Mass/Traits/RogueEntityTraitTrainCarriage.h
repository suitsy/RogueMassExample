// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityTraitBase.h"
#include "RogueEntityTraitTrainCarriage.generated.h"

/**
 * 
 */
UCLASS(EditInlineNew, meta=(DisplayName="Train Carriage Trait"))
class ROGUEMASSEXAMPLE_API URogueEntityTraitTrainCarriage : public UMassEntityTraitBase
{
	GENERATED_BODY()

protected:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override;
};
