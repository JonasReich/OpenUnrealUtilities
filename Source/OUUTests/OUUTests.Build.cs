// Copyright (c) 2021 Jonas Reich

using UnrealBuildTool;

public class OUUTests : ModuleRules
{
	public OUUTests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			// Engine
			"Core",
			"CoreUObject",
			"Engine",
			"UMG",

			// Plugin
			"OUURuntime",
			"GameplayTags"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
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