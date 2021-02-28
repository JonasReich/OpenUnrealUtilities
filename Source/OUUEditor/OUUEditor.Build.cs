// Copyright (c) 2021 Jonas Reich

using UnrealBuildTool;

public class OUUEditor : ModuleRules
{
	public OUUEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			// Engine
			"Core",
			"InputCore",
			"UMG",

			// Plugin
			"OUURuntime"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			// Engine
			"CoreUObject",
			"Engine",
			"Slate"
		});
	}
}