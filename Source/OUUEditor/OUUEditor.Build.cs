// Copyright (c) 2023 Jonas Reich & Contributors

using UnrealBuildTool;

public class OUUEditor : OUUModuleRules
{
	public OUUEditor(ReadOnlyTargetRules Target) : base(Target)
	{
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
			"Slate",
			"SlateCore",
			"MaterialEditor",
			"UnrealEd",
			"EditorStyle",
			"Projects",
			"SessionFrontend",
			"Blutility",
			"UMGEditor",
			"WorkspaceMenuStructure",
			"PropertyEditor",
			"SkeletalMeshEditor",
			"SkeletalMeshUtilitiesCommon",
			"EditorFramework",
			"LevelEditor",
			"ImageWriteQueue",
			"RenderCore",
			"PropertyEditor",
			"ContentBrowser",
			"Kismet",
			"ContentBrowserFileDataSource",
			"ContentBrowserData",
			"ToolMenus",
			"SourceControlWindows",
			"AssetTools",
			"SourceControl",
			"AssetRegistry",
			"Projects",
			"DataValidation",
			"EditorWidgets",
			"EditorSubsystem",
			"DeveloperSettings",
			"GameplayTags",
			"GameplayTagsEditor",
			"Json",
			"MessageLog",
			"WebBrowser",

			// OUU
			"OUURuntime",
			"OUUDeveloper",
			"OUUTags"
		});

		// Optional Hermes (tq2:// deep-link dispatch) support. Hermes is Win64-only and may be absent entirely
		// (e.g. when the OUU plugin is used in a project without it), so only depend on it when its module is
		// actually available. When enabled, the web browser widget forwards custom-scheme navigations to Hermes
		// in-process; otherwise those links are simply not intercepted.
		if (Target.Platform == UnrealTargetPlatform.Win64 && HasModule("HermesServer"))
		{
			PrivateDependencyModuleNames.Add("HermesServer");
			PrivateDefinitions.Add("WITH_OUU_HERMES=1");
		}
		else
		{
			PrivateDefinitions.Add("WITH_OUU_HERMES=0");
		}
	}

	private bool HasModule(string ModuleName)
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