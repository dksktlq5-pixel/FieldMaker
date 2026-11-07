// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class FieldMaker : ModuleRules
{
	public FieldMaker(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        // Keep translation-unit-local Slate helpers isolated, regardless of adaptive-unity Git state.
        bUseUnity = false;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });

		// Equipment shop uses UMG ownership with Slate layout and keyboard focus.
		PrivateDependencyModuleNames.AddRange(new string[] { "UMG", "Slate", "SlateCore", "AnimationCore", "Networking", "Sockets" });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
