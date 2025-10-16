// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Traits/RogueEntityTraitStation.h"
#include "MassEntityTemplateRegistry.h"
#include "Mass/Fragments/RogueFragments.h"

void URogueEntityTraitStation::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	BuildContext.AddTag<FRogueTrainStationTag>();
	BuildContext.AddFragment<FRogueStationRefFragment>();
	BuildContext.AddFragment<FRogueStationQueueFragment>();
}
