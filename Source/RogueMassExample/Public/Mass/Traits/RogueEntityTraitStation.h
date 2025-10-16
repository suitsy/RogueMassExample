// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityTraitBase.h"
#include "RogueEntityTraitStation.generated.h"

/**
 * 
 */
UCLASS(EditInlineNew, meta=(DisplayName="Station Trait"))
class ROGUEMASSEXAMPLE_API URogueEntityTraitStation : public UMassEntityTraitBase
{
	GENERATED_BODY()

protected:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override;
};
