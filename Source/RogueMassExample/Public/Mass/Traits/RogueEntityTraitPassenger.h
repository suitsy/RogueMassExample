// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityTraitBase.h"
#include "RogueEntityTraitPassenger.generated.h"

/**
 * 
 */
UCLASS(EditInlineNew, meta=(DisplayName="Train Passenger Trait"))
class ROGUEMASSEXAMPLE_API URogueEntityTraitPassenger : public UMassEntityTraitBase
{
	GENERATED_BODY()

protected:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override;
};
