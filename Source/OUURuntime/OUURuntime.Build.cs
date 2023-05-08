// Copyright (c) 2022 Jonas Reich

using UnrealBuildTool;

public class OUUModuleRuleHelpers
{
	public static void AddGameplayDebuggerDependency(ModuleRules Rules, ReadOnlyTargetRules Target)
	{
		if (Target.bBuildDeveloperTools || (Target.Configuration != UnrealTargetConfiguration.Shipping && Target.Configuration != UnrealTargetConfiguration.Test))
		{
			Rules.PrivateDependencyModuleNames.Add("GameplayDebugger");
			Rules.PublicDefinitions.Add("WITH_GAMEPLAY_DEBUGGER=1");
		}
		else
		{
			Rules.PublicDefinitions.Add("WITH_GAMEPLAY_DEBUGGER=0");
		}
	}
}

public class OUUModuleRules : ModuleRules
{
	public OUUModuleRules(ReadOnlyTargetRules Target) : base(Target)
	{
		// Disable PCHs for debug configs to ensure the plugin modules are self-contained and stick to IWYU
		PCHUsage = Target.Configuration == UnrealTargetConfiguration.DebugGame
			? ModuleRules.PCHUsageMode.NoPCHs
			: ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		bEnforceIWYU = true;

		// Also disable unity builds.
		// Unfortunately even this is needed to ensure that all includes are correct when building the plugin by itself.
		bUseUnity = false;

		OUUModuleRuleHelpers.AddGameplayDebuggerDependency(this, Target);
	}
}

public class OUURuntime : OUUModuleRules
{
	public OUURuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] {

			// Engine
			"CoreUObject",
			"Engine",
			"Core",
			"InputCore",
			"UMG",
			"SlateCore",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"DeveloperSettings"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {

			// Engine
			"HeadMountedDisplay",
			"Slate",
			"AIModule",
			"JsonUtilities",
			"Json",
			"Projects"
		});

		// - Editor only dependencies
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] {
				"UnrealEd",
				"GameplayTagsEditor",
				"PropertyEditor",
				"SourceControl",
				"AssetTools",

				"ContentBrowser",
				"ContentBrowserData",
				"AssetRegistry"
			});
		}
		// --
	}
}