// Copyright (c) 2022 Jonas Reich

#include "OUUContentBrowserExtensions.h"

#include "Algo/AllOf.h"
#include "ContentBrowserDelegates.h"
#include "ContentBrowserModule.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "OUUAnimationEditorLibrary.h"

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

		static TSharedRef<FExtender> OnExtendContentBrowserAssetSelectionMenu(const TArray<FAssetData>& SelectedAssets)
		{
			TSharedRef<FExtender> Extender(new FExtender());

			if (Algo::AllOf(SelectedAssets, [](const FAssetData& Asset) -> bool {
					return Asset.GetClass()->IsChildOf(USkeletalMesh::StaticClass());
				}))
			{
				Extender->AddMenuExtension(
					"ImportedAssetActions",
					EExtensionHook::Before,
					nullptr,
					FMenuExtensionDelegate::CreateLambda([SelectedAssets](FMenuBuilder& MenuBuilder) {
						MenuBuilder.BeginSection("OUUSkeletalMeshActions", INVTEXT("Open Unreal Utilities"));
						MenuBuilder.AddMenuEntry(
							INVTEXT("Remove Unskinned Bones"),
							INVTEXT("Remove all unskinned bones from the skeletal mesh.\n"
									"Warning: This does not filter out IK bones, etc!"),
							FSlateIcon(),
							FExecuteAction::CreateStatic(&RemoveUnskinnedBonesFromSelectedMeshes, SelectedAssets));
					}));
			}

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
