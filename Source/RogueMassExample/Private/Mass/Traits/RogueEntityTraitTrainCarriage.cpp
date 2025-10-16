// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Traits/RogueEntityTraitTrainCarriage.h"
#include "MassEntityTemplateRegistry.h"
#include "Mass/Fragments/RogueFragments.h"

void URogueEntityTraitTrainCarriage::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext,
                                                   const UWorld& World) const
{
	BuildContext.AddTag<FRogueTrainCarriageTag>();
	BuildContext.AddFragment<FRogueSplineFollowFragment>();
	BuildContext.AddFragment<FRogueTrainLinkFragment>();
	BuildContext.AddFragment<FRogueCarriageCapacityFragment>();
}
