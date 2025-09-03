using UnrealBuildTool;
using System.IO;

public class Quirc : ModuleRules
{
    public Quirc(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        bUseUnity = false; // opcional, mejor para depurar

        PublicDependencyModuleNames.AddRange(new[] { "Core" });

        // Headers públicos del módulo (incluye quirc.h)
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public"));
        //PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public", "ThirdParty", "quirc"));
		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "ThirdParty", "lib"));
        // Si quieres suprimir warnings del header de terceros:
        // PublicSystemIncludePaths.Add(Path.Combine(ModuleDirectory, "Public", "ThirdParty", "quirc"));


        CppStandard = CppStandardVersion.Cpp17; // afecta a .cpp; el .c se compila como C
    }
}