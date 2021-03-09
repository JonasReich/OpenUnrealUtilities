// Copyright (c) 2021 Jonas Reich

using UnrealBuildTool;

public class OUUTestUtilities : ModuleRules
{
	public OUUTestUtilities(ReadOnlyTargetRules Target) : base(Target)
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
			"OUURuntime"
		});
	}
}