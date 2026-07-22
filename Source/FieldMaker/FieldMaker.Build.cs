// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class FieldMaker : ModuleRules
{
	public FieldMaker(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",

				// Input
				"InputCore",
				"EnhancedInput",

				// Animation
				"AnimGraphRuntime",

				// UI
				"UMG",
				"Slate",
				"SlateCore",

				// AI
				"AIModule",
				"GameplayTasks",

				// Networking
				"NetCore"
			});

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{

			});

		PublicIncludePaths.Add(ModuleDirectory);
	}
}