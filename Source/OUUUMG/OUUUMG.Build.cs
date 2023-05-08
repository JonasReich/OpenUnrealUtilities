// Copyright (c) 2023 Jonas Reich & Contributors

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
