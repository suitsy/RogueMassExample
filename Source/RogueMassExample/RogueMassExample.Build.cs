// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class RogueMassExample : ModuleRules
{
	public RogueMassExample(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine",
			"MassEntity", 
			"MassCommon", 
			"MassMovement", 
			"MassSpawner", 
			"MassRepresentation",
			"MassNavigation",
			"MassCrowd",
			"MassLOD",
			"DeveloperSettings",
			"StructUtils"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {  });
	}
}
