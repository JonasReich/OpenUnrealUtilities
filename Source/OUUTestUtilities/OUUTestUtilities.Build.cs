// Copyright (c) 2023 Jonas Reich & Contributors

using UnrealBuildTool;

public class OUUTestUtilities : OUUModuleRules
{
	public OUUTestUtilities(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] {

			// Engine
			"Core",
			"CoreUObject",
			"Engine",
			"UMG",
			"AutomationController",
			"Gauntlet",
			"EngineSettings",
			"OnlineSubsystem",
			"OnlineSubsystemUtils",
			"FunctionalTesting",

			// Plugin
			"OUURuntime",
		});

		if (Target.bBuildEditor)
		{
			PublicDependencyModuleNames.Add("UnrealEd");
		}
	}
}
