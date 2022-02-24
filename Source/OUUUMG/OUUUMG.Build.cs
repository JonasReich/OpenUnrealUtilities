// Copyright (c) 2022 Jonas Reich

using UnrealBuildTool;

public class OUUUMG : OUUModuleRules
{
	public OUUUMG(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] {

			// Engine
			"CoreUObject",
			"Engine",
			"Core",
			"InputCore",
			"UMG",
			"SlateCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {

			// Engine
			"Slate",

			// Plugin
			"OUURuntime"
		});
	}
}
