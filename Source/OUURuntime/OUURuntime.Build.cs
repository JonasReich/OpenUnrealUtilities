// Copyright (c) 2021 Jonas Reich

using UnrealBuildTool;

public class OUURuntime : ModuleRules
{
	public OUURuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"CoreUObject",
			"Engine",
			"Core",
			"InputCore",
			"UMG",
			"SlateCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"HeadMountedDisplay",
			"Slate"
		});
	}
}