// Copyright (c) 2021 Jonas Reich

using UnrealBuildTool;

public class OUUDeveloper : ModuleRules
{
	public OUUDeveloper(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"CoreUObject",
			"Engine",
			"Core",
			"InputCore",
			"UMG",
			"SlateCore",

			"OUURuntime",
			"OUUUMG"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"AIModule",
			"GameplayTags",
			"GameplayAbilities"
		});

		// - Editor only dependencies
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.Add("UnrealEd");
		}
		// --

		// #TODO-j.reich Figure out if it makes sense to move the gameplay debugger stuff into this module as well!
		if (Target.bBuildDeveloperTools || (Target.Configuration != UnrealTargetConfiguration.Shipping && Target.Configuration != UnrealTargetConfiguration.Test))
		{
			PublicDependencyModuleNames.Add("GameplayDebugger");
			PublicDefinitions.Add("WITH_GAMEPLAY_DEBUGGER=1");
		}
		else
		{
			PublicDefinitions.Add("WITH_GAMEPLAY_DEBUGGER=0");
		}
	}
}