using UnrealBuildTool;

public class RogueAIDebugger : ModuleRules
{
    public RogueAIDebugger(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core", 
                "RogueMassExample",
                "GameplayDebugger",
                "GameplayDebuggerEditor"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",  
                "AIModule",
                "DeveloperSettings", 
                "GameplayTags",
                "InputCore", 
            }
        );
    }
}