// Copyright (c) 2023 Jonas Reich & Contributors

using System;
using EpicGames.Core;
using UnrealBuildTool;

public class OUUModuleRuleHelpers
{
	public static void AddGameplayDebuggerDependency(ModuleRules Rules, ReadOnlyTargetRules Target)
	{
		if (Target.bBuildDeveloperTools || (Target.Configuration != UnrealTargetConfiguration.Shipping &&
		                                    Target.Configuration != UnrealTargetConfiguration.Test))
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
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		IWYUSupport = IWYUSupport.Full;

		// Also disable unity builds.
		// Unfortunately even this is needed to ensure that all includes are correct when building the plugin by itself.
		bUseUnity = false;

#if UE_5_4_OR_LATER
		bWarningsAsErrors = true;
#endif

		OUUModuleRuleHelpers.AddGameplayDebuggerDependency(this, Target);
	}
}

public class OUURuntime : OUUModuleRules
{
	public OUURuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[]
		{
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
			"DeveloperSettings",
			"CommonUI",
			
			// OUU Tags
			"OUUTags"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			// Engine
			"HeadMountedDisplay",
			"Slate",
			"AIModule",
			"JsonUtilities",
			"Json",
			"Projects",
			"NetCore"
		});

		// - Editor only dependencies
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[]
			{
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

		if (HasModule("BFGTickOptimizer"))
		{
			PublicDefinitions.Add("WITH_BFG_TICK_OPTIMIZER=1");
		}
		else
		{
			PublicDefinitions.Add("WITH_BFG_TICK_OPTIMIZER=0");
		}
		// --
	}

	bool HasModule(string ModuleName)
	{
		try
		{
			GetModuleDirectory(ModuleName);
			return true;
		}
		catch (BuildException e)
		{
			if (e.Message.Contains("Could not find a module named"))
			{
				return false;
			}

			throw;
		}
	}
}