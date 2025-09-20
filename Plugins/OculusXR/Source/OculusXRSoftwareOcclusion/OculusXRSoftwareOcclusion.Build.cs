// @lint-ignore-every LICENSELINT
// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class OculusXRSoftwareOcclusion : ModuleRules
{
    public OculusXRSoftwareOcclusion(ReadOnlyTargetRules Target) : base(Target)
    {
        bUseUnity = true;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "RenderCore"
            }
        );

        var EngineDir = Path.GetFullPath(Target.RelativeEnginePath);

        

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Renderer"
        });

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(new string[]
            {
                "UnrealEd"
            });
        }

        // FSceneSoftwareOcclusion needs to access FScene, etc. of Renderer.
        PrivateIncludePaths.AddRange(
            new string[] {
                    Path.Combine(EngineDir, "Source/Runtime/Renderer/Private"),
                    Path.Combine(EngineDir, "Source/Runtime/Renderer/Internal"),
            });
    }
}
