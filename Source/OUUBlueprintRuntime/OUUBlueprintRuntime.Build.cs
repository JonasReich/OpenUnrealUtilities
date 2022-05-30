// Copyright (c) 2022 Jonas Reich

using UnrealBuildTool;

public class OUUBlueprintRuntime : OUUModuleRules
{
	public OUUBlueprintRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] {

			// Engine
			"Core",
			"InputCore",
			"UMG"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {

			// Engine
			"CoreUObject",
			"Engine",
			"Slate",
			"SlateCore",
			"EngineSettings",
			"HeadMountedDisplay",
			"PropertyPath",
			"ApplicationCore",
			"EditorScriptingUtilities",

			// Plugin
			"OUURuntime"
		});
	}
}
