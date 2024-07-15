// Copyright (c) 2023 Jonas Reich & Contributors

#include "OUUContentBrowserExtensions.h"

#include "Algo/AllOf.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "ContentBrowserDataSubsystem.h"
#include "ContentBrowserDelegates.h"
#include "ContentBrowserModule.h"
#include "Engine/SkeletalMesh.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "IContentBrowserSingleton.h"
#include "LogOpenUnrealUtilities.h"
#include "Misc/FeedbackContext.h"
#include "OUUAnimationEditorLibrary.h"
#include "UObject/SavePackage.h"

namespace OUU::Editor::ContentBrowserExtensions
{
	namespace Private
	{
		static FContentBrowserMenuExtender_SelectedAssets ContentBrowserExtenderDelegate;
		static FDelegateHandle ContentBrowserExtenderDelegateHandle;

		TArray<FContentBrowserMenuExtender_SelectedAssets>& GetExtenderDelegates()
		{
			FContentBrowserModule& ContentBrowserModule =
				FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
			return ContentBrowserModule.GetAllAssetViewContextMenuExtenders();
		}

		static void RemoveUnskinnedBonesFromSelectedMeshes(TArray<FAssetData> SelectedAssets)
		{
			for (const FAssetData& Asset : SelectedAssets)
			{
				if (auto* SkeletalMesh = Cast<USkeletalMesh>(Asset.GetAsset()))
				{
					UOUUAnimationEditorLibrary::RemoveUnskinnedBonesFromMesh(SkeletalMesh, "", "", 0);
				}
			}
		}

		static void CreateRedirector(UObject* TargetAsset, const FString& RedirectorObjectPath)
		{
			if (ensureMsgf(TargetAsset, TEXT("Expected valid redirector target asset")) == false)
			{
				return;
			}

			const auto RedirectorPackageName = FPackageName::ObjectPathToPackageName(RedirectorObjectPath);

			FString DiscardPackageFilename;
			if (FPackageName::DoesPackageExist(RedirectorPackageName, OUT & DiscardPackageFilename))
			{
				return;
			}

			// Create the package for the redirector
			const FString RedirectorObjectName = FPackageName::GetLongPackageAssetName(RedirectorPackageName);
			UPackage* RedirectorPackage = CreatePackage(*RedirectorPackageName);

			// Create the redirector itself
			UObjectRedirector* Redirector = NewObject<UObjectRedirector>(
				RedirectorPackage,
				FName(*RedirectorObjectName),
				RF_Standalone | RF_Public);
			Redirector->DestinationObject = TargetAsset;

			FAssetRegistryModule::AssetCreated(Redirector);

			// Save the package
			auto PackageFilename = FPackageName::LongPackageNameToFilename(
				RedirectorPackageName,
				FPackageName::GetAssetPackageExtension());

			FString BaseFilename, Extension, Directory;
			FPaths::NormalizeFilename(PackageFilename);
			FPaths::Split(PackageFilename, OUT Directory, OUT BaseFilename, OUT Extension);

			FSavePackageArgs SaveArgs;
			SaveArgs.TopLevelFlags = RF_Standalone;
			SaveArgs.Error = GWarn;
			const auto SaveResult = UPackage::Save(RedirectorPackage, Redirector, *PackageFilename, SaveArgs);

			UE_LOG(
				LogOpenUnrealUtilities,
				Log,
				TEXT("Created asset redirector: %s -> %s"),
				*RedirectorPackageName,
				*GetNameSafe(TargetAsset));
		}

		static void CreateRedirectors(TArray<FAssetData> SelectedAssets)
		{
			for (auto& Asset : SelectedAssets)
			{
				FSaveAssetDialogConfig Config;
				Config.DefaultPath = Asset.PackagePath.ToString();
				Config.DefaultAssetName = Asset.AssetName.ToString() + TEXT("_Redirector");
				Config.ExistingAssetPolicy = ESaveAssetDialogExistingAssetPolicy::Disallow;

				auto SelectedObjectPath = IContentBrowserSingleton::Get().CreateModalSaveAssetDialog(Config);

				if (SelectedObjectPath.IsEmpty() == false)
				{
					CreateRedirector(Asset.GetAsset(), SelectedObjectPath);
				}
			}
		}

		static TSharedRef<FExtender> OnExtendContentBrowserAssetSelectionMenu(const TArray<FAssetData>& SelectedAssets)
		{
			TSharedRef<FExtender> Extender(new FExtender());

			if (Algo::AllOf(SelectedAssets, [](const FAssetData& Asset) -> bool {
					return (Asset.GetClass() != nullptr) && Asset.GetClass()->IsChildOf(USkeletalMesh::StaticClass());
				}))
			{
				Extender->AddMenuExtension(
					TEXT("ImportedAssetActions"),
					EExtensionHook::Before,
					nullptr,
					FMenuExtensionDelegate::CreateLambda([SelectedAssets](FMenuBuilder& MenuBuilder) {
						MenuBuilder.BeginSection(TEXT("OUUSkeletalMeshActions"), INVTEXT("Open Unreal Utilities"));
						MenuBuilder.AddMenuEntry(
							INVTEXT("Remove Unskinned Bones"),
							INVTEXT("Remove all unskinned bones from the skeletal mesh.\n"
									"Warning: This does not filter out IK bones, etc!"),
							FSlateIcon(),
							FExecuteAction::CreateStatic(&RemoveUnskinnedBonesFromSelectedMeshes, SelectedAssets));
					}));
			}

			Extender->AddMenuExtension(
				TEXT("CommonAssetActions"),
				EExtensionHook::Before,
				nullptr,
				FMenuExtensionDelegate::CreateLambda([SelectedAssets](FMenuBuilder& MenuBuilder) {
					MenuBuilder.BeginSection(TEXT("OUURedirectorActions"), INVTEXT("OUU Redirector Actions"));
					MenuBuilder.AddMenuEntry(
						INVTEXT("Create Redirector"),
						INVTEXT("Create an asset redirector pointing to the selected asset(s)"),
						FSlateIcon(FAppStyle::Get().GetStyleSetName(), "Icons.PlusCircle"),
						FExecuteAction::CreateStatic(&CreateRedirectors, SelectedAssets));
				}));

			return Extender;
		}

	} // namespace Private

	void RegisterHooks()
	{
		Private::ContentBrowserExtenderDelegate = FContentBrowserMenuExtender_SelectedAssets::CreateStatic(
			&Private::OnExtendContentBrowserAssetSelectionMenu);

		TArray<FContentBrowserMenuExtender_SelectedAssets>& CBMenuExtenderDelegates = Private::GetExtenderDelegates();
		CBMenuExtenderDelegates.Add(Private::ContentBrowserExtenderDelegate);
		Private::ContentBrowserExtenderDelegateHandle = CBMenuExtenderDelegates.Last().GetHandle();
	}

	void UnregisterHooks()
	{
		TArray<FContentBrowserMenuExtender_SelectedAssets>& CBMenuExtenderDelegates = Private::GetExtenderDelegates();
		CBMenuExtenderDelegates.RemoveAll([](const FContentBrowserMenuExtender_SelectedAssets& Delegate) {
			return Delegate.GetHandle() == Private::ContentBrowserExtenderDelegateHandle;
		});
		Private::ContentBrowserExtenderDelegateHandle.Reset();
	}
} // namespace OUU::Editor::ContentBrowserExtensions
