// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Cam2Android : ModuleRules
{
	public Cam2Android(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "AndroidCamera2UECore","Quirc"});

		PrivateDependencyModuleNames.AddRange(new string[] {  });

		
	}
}
