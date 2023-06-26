// Copyright (c) 2023 Jonas Reich & Contributors

using UnrealBuildTool;

public class OUUTests : OUUModuleRules
{
	public OUUTests(ReadOnlyTargetRules Target) : base(Target)
	{
		// No public dependencies: This module does not have any public files.

		PrivateDependencyModuleNames.AddRange(new string[] {

			// Engine
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"UMG",
			"SlateCore",
			"Localization",
			"GameplayTags",
			"Json",
			"PropertyPath",

			// OUU Plugins
			"OUURuntime",
			"OUUBlueprintRuntime",
			"OUUUMG",
			"OUUTestUtilities"
		});

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] {
				"EditorScriptingUtilities",
			});
		}
	}
}
