using UnrealBuildTool;
using System.IO;

public class Quirc : ModuleRules
{
    public Quirc(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        bUseUnity = false; 

        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject" , "Engine"});

        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public"));
		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "ThirdParty", "lib"));
    }
}