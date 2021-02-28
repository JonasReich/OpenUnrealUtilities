// Copyright (c) 2021 Jonas Reich

using UnrealBuildTool;

public class OUUBlueprintRuntime : ModuleRules
{
	public OUUBlueprintRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"InputCore",
				"UMG"
			});

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
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
