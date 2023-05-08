// Copyright (c) 2023 Jonas Reich & Contributors

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

			// Plugin
			"OUURuntime"
		});
	}
}
