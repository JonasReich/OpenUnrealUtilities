// Copyright (c) 2022 Jonas Reich

using UnrealBuildTool;

public class OUUEditor : OUUModuleRules
{
	public OUUEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] {

			// Engine
			"Core",
			"InputCore",
			"UMG",

			// Plugin
			"OUURuntime"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {

			// Engine
			"CoreUObject",
			"Engine",
			"Slate"
		});
	}
}
