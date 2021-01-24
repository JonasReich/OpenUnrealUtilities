// Copyright (c) 2021 Jonas Reich

using UnrealBuildTool;

public class OUUTests : ModuleRules
{
	public OUUTests(ReadOnlyTargetRules Target) : base(Target)
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
				"OUURuntime"
			}
			);
	}
}
