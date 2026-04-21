// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TestTast : ModuleRules
{
	public TestTast(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "GameplayTags" });

		PrivateDependencyModuleNames.AddRange(new string[] {  });
	}
}
