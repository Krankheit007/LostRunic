// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class LostRunic : ModuleRules
{
	public LostRunic(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"EnhancedInput",
			"AIModule",
			"DeveloperSettings",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"GameplayTags",
			"UMG",
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"InputCore",
			"NavigationSystem",
			"GameplayTasks",
			"Niagara",
			"PhysicsCore",
			"Slate",
			"SlateCore"
		});

		PublicIncludePaths.AddRange(new string[] {
			"LostRunic",
			"LostRunic/Variant_Strategy",
			"LostRunic/Variant_Strategy/UI",
			"LostRunic/Variant_TwinStick",
			"LostRunic/Variant_TwinStick/AI",
			"LostRunic/Variant_TwinStick/Gameplay",
			"LostRunic/Variant_TwinStick/UI"
		});

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
