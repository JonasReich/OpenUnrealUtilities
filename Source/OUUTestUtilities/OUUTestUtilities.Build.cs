// Copyright (c) 2022 Jonas Reich

using UnrealBuildTool;

public class OUUTestUtilities : OUUModuleRules
{
	public OUUTestUtilities(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] {

			// Engine
			"Core",
			"CoreUObject",
			"Engine",
			"UMG",
			"SignificanceManager",
			"AutomationController",

			// Plugin
			"OUURuntime"
		});
	}
}
