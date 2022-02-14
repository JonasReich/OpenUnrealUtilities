// Copyright (c) 2022 Jonas Reich

using UnrealBuildTool;

public class OUUBlueprintRuntime : ModuleRules
{
	public OUUBlueprintRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

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

			// Plugin
			"OUURuntime"
		});
	}
}
