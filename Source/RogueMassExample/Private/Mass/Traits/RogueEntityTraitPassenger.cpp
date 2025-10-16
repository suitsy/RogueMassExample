// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Traits/RogueEntityTraitPassenger.h"
#include "MassEntityTemplateRegistry.h"
#include "Mass/Fragments/RogueFragments.h"

void URogueEntityTraitPassenger::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	BuildContext.AddTag<FRogueTrainPassengerTag>();
	BuildContext.AddFragment<FRoguePassengerRouteFragment>();
	BuildContext.AddFragment<FRogueStationRefFragment>();
}
