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

			// OUU
			"OUURuntime",
			"OUUDeveloper",
		});
	}
}