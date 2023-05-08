// Copyright (c) 2023 Jonas Reich & Contributors

using UnrealBuildTool;

public class OUUDeveloper : OUUModuleRules
{
	public OUUDeveloper(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] {

			// Engine
			"CoreUObject",
			"Engine",
			"Core",
			"InputCore",
			"UMG",
			"SlateCore",

			// Plugin
			"OUURuntime",
			"OUUUMG"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {

			// Engine
			"Slate",
			"AIModule",
			"GameplayTags",
			"GameplayAbilities",
			"DeveloperSettings",
			"EngineSettings",
			"SourceControl"
		});

		// - Editor only dependencies
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] {

				// Engine
				"UnrealEd",
				"EditorStyle",
				"WorkspaceMenuStructure"
			});
		}
		// --

		// #TODO-OUU Figure out if it makes sense to move the other gameplay debugger stuff into this module as well!
		OUUModuleRuleHelpers.AddGameplayDebuggerDependency(this, Target);
	}
}
