using UnrealBuildTool;

public class GorillaProtocol : ModuleRules
{
    public GorillaProtocol(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "EnhancedInput",
            "GameplayTags",
            "UMG"
        });
    }
}
