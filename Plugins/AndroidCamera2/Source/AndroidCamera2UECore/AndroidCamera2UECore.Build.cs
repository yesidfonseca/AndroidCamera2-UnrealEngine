// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 Yesid Fonseca

using System.IO;
using UnrealBuildTool;

public class AndroidCamera2UECore : ModuleRules
{
    public AndroidCamera2UECore(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "RenderCore",
            "RHI",
            "Projects",   // IPluginManager
			"RenderCore",  // AddShaderSourceDirectoryMapping
            "DeveloperSettings"
        });


        if (Target.Platform == UnrealTargetPlatform.Android)
        {
            PrivateDependencyModuleNames.Add("AndroidCamera2");
        }
    }
}