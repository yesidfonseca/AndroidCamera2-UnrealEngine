using UnrealBuildTool;
using System.IO;

public class libyuv : ModuleRules
{
    public libyuv(ReadOnlyTargetRules Target) : base(Target)
    {
        Type = ModuleType.External;

        string ThirdPartyPath = Path.GetFullPath(Path.Combine(ModuleDirectory, ".")); // .../ThirdParty/libyuv
        string IncludePath    = Path.Combine(ThirdPartyPath, "include");
        PublicSystemIncludePaths.Add(IncludePath); // headers: libyuv/...

        if (Target.Platform == UnrealTargetPlatform.Android)
        {
            // Solo soportaremos ARM64 (recomendado en UE 5.x)
            string LibPath = Path.Combine(ThirdPartyPath, "lib", "Android", "ARM64");
            PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libyuv.a"));

            PublicDefinitions.Add("WITH_LIBYUV=1");
        }
        else
        {
            // Editor/otras plataformas: compila sin libyuv
            PublicDefinitions.Add("WITH_LIBYUV=0");
        }
    }
}