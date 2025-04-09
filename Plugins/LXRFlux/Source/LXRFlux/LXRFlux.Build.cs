using UnrealBuildTool;

public class LXRFlux: ModuleRules
{
    public LXRFlux(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "RHI",
                "RenderCore",
                "Projects",
                "Renderer",
                "UnrealEd",
                "UMG"

            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore"
                
            }
        );
    }
}