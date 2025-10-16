// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Traits/RogueEntityTraitTrainEngine.h"
#include "MassEntityTemplateRegistry.h"
#include "Mass/Fragments/RogueFragments.h"

void URogueEntityTraitTrainEngine::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	BuildContext.AddTag<FRogueTrainEngineTag>();
	BuildContext.AddFragment<FRogueSplineFollowFragment>();
	BuildContext.AddFragment<FRogueTrainStateFragment>();
}
