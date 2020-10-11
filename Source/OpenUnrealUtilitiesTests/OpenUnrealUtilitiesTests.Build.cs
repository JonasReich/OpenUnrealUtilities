// Copyright (c) 2020 Jonas Reich

using UnrealBuildTool;

public class OpenUnrealUtilitiesTests : ModuleRules
{
	public OpenUnrealUtilitiesTests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				// Engine
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
				"SlateCore",
				"UMG",
				"EditorScriptingUtilities",

				// Plugin
				"OpenUnrealUtilities"
			}
			);
	}
}
