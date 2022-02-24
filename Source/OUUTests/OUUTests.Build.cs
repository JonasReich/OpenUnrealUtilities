// Copyright (c) 2022 Jonas Reich

using UnrealBuildTool;

public class OUUTests : OUUModuleRules
{
	public OUUTests(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] {

			// Engine
			"Core",
			"CoreUObject",
			"Engine",
			"UMG",

			// Plugin
			"OUURuntime",
			"GameplayTags"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {

			// Engine
			"InputCore",
			"SlateCore",
			"EditorScriptingUtilities",

			// Plugin
			"OUUBlueprintRuntime",
			"OUUUMG",
			"OUUTestUtilities"
		});
	}
}
