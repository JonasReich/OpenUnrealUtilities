// Copyright (c) 2022 Jonas Reich

#include "JsonDataAsset/JsonFileSourceControlContextMenu.h"

#include "AssetViewUtils.h"
#include "ContentBrowserFileDataPayload.h"
#include "ISourceControlModule.h"
#include "JsonDataAsset/JsonDataAsset.h"
#include "JsonDataAsset/JsonDataAssetEditor.h"
#include "SourceControlHelpers.h"
#include "SourceControlOperations.h"
#include "SourceControlWindows.h"
#include "ToolMenu.h"

#define LOCTEXT_NAMESPACE "OUU_Json_ContentBrowser"

void FJsonFileSourceControlContextMenu::MakeContextMenu(
	UToolMenu* InMenu,
	TArray<TSharedRef<const FContentBrowserFileItemDataPayload>>& InSelectedAssets)
{
	SelectedAssets = InSelectedAssets;

	if (SelectedAssets.Num() > 0)
	{
		AddMenuOptions(InMenu);
	}
}

void FJsonFileSourceControlContextMenu::AddSourceControlMenuOptions(UToolMenu* Menu)
{
	FToolMenuSection& Section = Menu->AddSection("AssetContextSourceControl");

	if (ISourceControlModule::Get().IsEnabled())
	{
		// SCC sub menu
		Section.AddSubMenu(
			"SourceControlSubMenu",
			LOCTEXT("SourceControlSubMenuLabel", "Source Control"),
			LOCTEXT("SourceControlSubMenuToolTip", "Source control actions."),
			FNewToolMenuDelegate::CreateSP(this, &FJsonFileSourceControlContextMenu::FillSourceControlSubMenu),
			FUIAction(
				FExecuteAction(),
				FCanExecuteAction::CreateSP(this, &FJsonFileSourceControlContextMenu::CanExecuteSourceControlActions)),
			EUserInterfaceActionType::Button,
			false,
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "SourceControl.StatusIcon.On"));
	}
	else
	{
		Section.AddMenuEntry(
			"SCCConnectToSourceControl",
			LOCTEXT("SCCConnectToSourceControl", "Connect To Source Control..."),
			LOCTEXT(
				"SCCConnectToSourceControlTooltip",
				"Connect to source control to allow source control operations to be performed on content and "
				"levels."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "MainFrame.ConnectToSourceControl"),
			FUIAction(
				FExecuteAction::CreateSP(this, &FJsonFileSourceControlContextMenu::ExecuteEnableSourceControl),
				FCanExecuteAction::CreateSP(this, &FJsonFileSourceControlContextMenu::CanExecuteSourceControlActions)));
	}

	// Diff selected
	if (CanExecuteDiffSelected())
	{
		Section.AddMenuEntry(
			"DiffSelected",
			LOCTEXT("DiffSelected", "Diff Selected"),
			LOCTEXT("DiffSelectedTooltip", "Diff the two assets that you have selected."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "SourceControl.Actions.Diff"),
			FUIAction(FExecuteAction::CreateSP(this, &FJsonFileSourceControlContextMenu::ExecuteDiffSelected)));
	}
}

void FJsonFileSourceControlContextMenu::AddMenuOptions(UToolMenu* Menu)
{
	const UContentBrowserDataMenuContext_FileMenu* Context =
		Menu->FindContext<UContentBrowserDataMenuContext_FileMenu>();
	checkf(Context, TEXT("Required context UContentBrowserDataMenuContext_FileMenu was missing!"));

	ParentWidget = Context->ParentWidget;
	OnRefreshView = Context->OnRefreshView;

	// Cache any vars that are used in determining if you can execute any actions.
	// Useful for actions whose "CanExecute" will not change or is expensive to calculate.
	CacheCanExecuteVars();

	if (Context->bCanBeModified)
	{
		AddSourceControlMenuOptions(Menu);
	}
}

void FJsonFileSourceControlContextMenu::FillSourceControlSubMenu(UToolMenu* Menu)
{
	FToolMenuSection& Section = Menu->AddSection(
		"AssetSourceControlActions",
		LOCTEXT("AssetSourceControlActionsMenuHeading", "Source Control"));

	if (bCanExecuteSCCSync)
	{
		Section.AddMenuEntry(
			"SCCSync",
			LOCTEXT("SCCSync", "Sync"),
			LOCTEXT("SCCSyncTooltip", "Updates the item to the latest version in source control."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "SourceControl.Actions.Sync"),
			FUIAction(
				FExecuteAction::CreateSP(this, &FJsonFileSourceControlContextMenu::ExecuteSCCSync),
				FCanExecuteAction::CreateLambda([this]() { return bCanExecuteSCCSync; })));
	}

	if (bCanExecuteSCCCheckOut)
	{
		Section.AddMenuEntry(
			"SCCCheckOut",
			LOCTEXT("SCCCheckOut", "Check Out"),
			LOCTEXT("SCCCheckOutTooltip", "Checks out the selected asset from source control."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "SourceControl.Actions.CheckOut"),
			FUIAction(
				FExecuteAction::CreateSP(this, &FJsonFileSourceControlContextMenu::ExecuteSCCCheckOut),
				FCanExecuteAction::CreateLambda([this]() { return bCanExecuteSCCCheckOut; })));
	}

	if (bCanExecuteSCCOpenForAdd)
	{
		Section.AddMenuEntry(
			"SCCOpenForAdd",
			LOCTEXT("SCCOpenForAdd", "Mark For Add"),
			LOCTEXT("SCCOpenForAddTooltip", "Adds the selected asset to source control."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "SourceControl.Actions.Add"),
			FUIAction(
				FExecuteAction::CreateSP(this, &FJsonFileSourceControlContextMenu::ExecuteSCCOpenForAdd),
				FCanExecuteAction::CreateLambda([this]() { return bCanExecuteSCCOpenForAdd; })));
	}

	if (bCanExecuteSCCCheckIn)
	{
		Section.AddMenuEntry(
			"SCCCheckIn",
			LOCTEXT("SCCCheckIn", "Check In"),
			LOCTEXT("SCCCheckInTooltip", "Checks in the selected asset to source control."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "SourceControl.Actions.Submit"),
			FUIAction(
				FExecuteAction::CreateSP(this, &FJsonFileSourceControlContextMenu::ExecuteSCCCheckIn),
				FCanExecuteAction::CreateLambda([this]() { return bCanExecuteSCCCheckIn; })));
	}

	Section.AddMenuEntry(
		"SCCRefresh",
		LOCTEXT("SCCRefresh", "Refresh"),
		LOCTEXT("SCCRefreshTooltip", "Updates the source control status of the asset."),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "SourceControl.Actions.Refresh"),
		FUIAction(
			FExecuteAction::CreateSP(this, &FJsonFileSourceControlContextMenu::ExecuteSCCRefresh),
			FCanExecuteAction::CreateSP(this, &FJsonFileSourceControlContextMenu::CanExecuteSCCRefresh)));

	if (bCanExecuteSCCHistory)
	{
		Section.AddMenuEntry(
			"SCCHistory",
			LOCTEXT("SCCHistory", "History"),
			LOCTEXT("SCCHistoryTooltip", "Displays the source control revision history of the selected asset."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "SourceControl.Actions.History"),
			FUIAction(
				FExecuteAction::CreateSP(this, &FJsonFileSourceControlContextMenu::ExecuteSCCHistory),
				FCanExecuteAction::CreateLambda([this]() { return bCanExecuteSCCHistory; })));

		Section.AddMenuEntry(
			"SCCDiffAgainstDepot",
			LOCTEXT("SCCDiffAgainstDepot", "Diff Against Depot"),
			LOCTEXT(
				"SCCDiffAgainstDepotTooltip",
				"Look at differences between your version of the asset and that in source control."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "SourceControl.Actions.Diff"),
			FUIAction(
				FExecuteAction::CreateSP(this, &FJsonFileSourceControlContextMenu::ExecuteSCCDiffAgainstDepot),
				FCanExecuteAction::CreateLambda([this]() { return bCanExecuteSCCHistory; })));
	}

	if (bCanExecuteSCCRevert)
	{
		Section.AddMenuEntry(
			"SCCRevert",
			LOCTEXT("SCCRevert", "Revert"),
			LOCTEXT("SCCRevertTooltip", "Reverts the asset to the state it was before it was checked out."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "SourceControl.Actions.Revert"),
			FUIAction(
				FExecuteAction::CreateSP(this, &FJsonFileSourceControlContextMenu::ExecuteSCCRevert),
				FCanExecuteAction::CreateLambda([this]() { return bCanExecuteSCCRevert; })));
	}
}

void FJsonFileSourceControlContextMenu::CacheCanExecuteVars()
{
	bCanExecuteSCCCheckOut = false;
	bCanExecuteSCCOpenForAdd = false;
	bCanExecuteSCCCheckIn = false;
	bCanExecuteSCCHistory = false;
	bCanExecuteSCCRevert = false;
	bCanExecuteSCCSync = false;

	for (auto Asset : SelectedAssets)
	{
		ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
		if (ISourceControlModule::Get().IsEnabled())
		{
			// Check the SCC state for each package in the selected paths
			FSourceControlStatePtr SourceControlState =
				SourceControlProvider.GetState(Asset->GetFilename(), EStateCacheUsage::Use);
			if (SourceControlState.IsValid())
			{
				if (SourceControlState->CanCheckout())
				{
					bCanExecuteSCCCheckOut = true;
				}

				if (!SourceControlState->IsSourceControlled() && SourceControlState->CanAdd())
				{
					bCanExecuteSCCOpenForAdd = true;
				}
				else if (SourceControlState->IsSourceControlled() && !SourceControlState->IsAdded())
				{
					bCanExecuteSCCHistory = true;
				}

				if (!SourceControlState->IsCurrent() && SourceControlProvider.UsesFileRevisions())
				{
					bCanExecuteSCCSync = true;
				}

				if (SourceControlState->CanCheckIn())
				{
					bCanExecuteSCCCheckIn = true;
				}

				if (SourceControlState->CanRevert())
				{
					bCanExecuteSCCRevert = true;
				}
			}
		}

		if (bCanExecuteSCCCheckOut && bCanExecuteSCCOpenForAdd && bCanExecuteSCCCheckIn && bCanExecuteSCCHistory
			&& bCanExecuteSCCRevert && bCanExecuteSCCSync)
		{
			// All options are available, no need to keep iterating
			break;
		}
	}
}

bool FJsonFileSourceControlContextMenu::CanExecuteSourceControlActions() const
{
	return SelectedAssets.Num() > 0;
}

bool FJsonFileSourceControlContextMenu::CanExecuteSCCRefresh() const
{
	return ISourceControlModule::Get().IsEnabled();
}

bool FJsonFileSourceControlContextMenu::CanExecuteDiffSelected() const
{
	return SelectedAssets.Num() == 2;
}

void FJsonFileSourceControlContextMenu::ExecuteEnableSourceControl()
{
	ISourceControlModule::Get().ShowLoginDialog(FSourceControlLoginClosed(), ELoginWindowMode::Modeless);
}

void FJsonFileSourceControlContextMenu::ExecuteDiffSelected() const
{
	if (SelectedAssets.Num() == 2)
	{
		OUU::Editor::JsonData::PerformDiff(
			OUU::Editor::JsonData::ConvertMountedSourceFilenameToDataAssetPath(SelectedAssets[0]->GetFilename()),
			OUU::Editor::JsonData::ConvertMountedSourceFilenameToDataAssetPath(SelectedAssets[1]->GetFilename()));
	}
}

void FJsonFileSourceControlContextMenu::ExecuteSCCRefresh()
{
	TArray<FString> PackageNames;
	GetSelectedPackageNames(PackageNames);

	ISourceControlModule::Get().GetProvider().Execute(
		ISourceControlOperation::Create<FUpdateStatus>(),
		SourceControlHelpers::PackageFilenames(PackageNames),
		EConcurrency::Asynchronous);
}

void FJsonFileSourceControlContextMenu::ExecuteSCCCheckOut()
{
	TArray<FString> PackageNames;
	GetSelectedPackageNames(PackageNames);

	if (PackageNames.Num() > 0)
	{
		SourceControlHelpers::CheckOutFiles(PackageNames);
	}
}

void FJsonFileSourceControlContextMenu::ExecuteSCCOpenForAdd()
{
	TArray<FString> PackageNames;
	GetSelectedPackageNames(PackageNames);

	if (PackageNames.Num() > 0)
	{
		SourceControlHelpers::MarkFilesForAdd(PackageNames);
	}
}

void FJsonFileSourceControlContextMenu::ExecuteSCCCheckIn()
{
	/*
	// Prompt the user to ask if they would like to first save any dirty packages they are trying to check-in
	const FEditorFileUtils::EPromptReturnCode UserResponse =
		FEditorFileUtils::PromptForCheckoutAndSave(Packages, true, true);

	// If the user elected to save dirty packages, but one or more of the packages failed to save properly OR if the
	// user canceled out of the prompt, don't follow through on the check-in process
	const bool bShouldProceed =
		(UserResponse == FEditorFileUtils::EPromptReturnCode::PR_Success
		 || UserResponse == FEditorFileUtils::EPromptReturnCode::PR_Declined);
	if (bShouldProceed)
	{
		TArray<FString> PackageNames;
		GetSelectedPackageNames(PackageNames);

		const bool bUseSourceControlStateCache = true;

		FCheckinResultInfo ResultInfo;
		FSourceControlWindows::PromptForCheckin(
			ResultInfo,
			PackageNames,
			TArray<FString>(),
			TArray<FString>(),
			bUseSourceControlStateCache);

		if (ResultInfo.Result == ECommandResult::Failed)
		{
			FMessageDialog::Open(
				EAppMsgType::Ok,
				NSLOCTEXT("UnrealEd", "SCC_Checkin_Failed", "Check-in failed as a result of save failure."));
		}
	}
	else
	{
		// If a failure occurred, alert the user that the check-in was aborted. This warning shouldn't be necessary
		// if the user cancelled from the dialog, because they obviously intended to cancel the whole operation.
		if (UserResponse == FEditorFileUtils::EPromptReturnCode::PR_Failure)
		{
			FMessageDialog::Open(
				EAppMsgType::Ok,
				NSLOCTEXT("UnrealEd", "SCC_Checkin_Aborted", "Check-in aborted as a result of save failure."));
		}
	}
		*/
}

void FJsonFileSourceControlContextMenu::ExecuteSCCHistory()
{
	TArray<FString> PackageNames;
	GetSelectedPackageNames(PackageNames);
	FSourceControlWindows::DisplayRevisionHistory(SourceControlHelpers::PackageFilenames(PackageNames));
}

void DiffAgainstDepot(const FString& Filename)
{
	// Make sure our history is up to date
	ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
	TSharedRef<FUpdateStatus, ESPMode::ThreadSafe> UpdateStatusOperation =
		ISourceControlOperation::Create<FUpdateStatus>();
	UpdateStatusOperation->SetUpdateHistory(true);
	SourceControlProvider.Execute(UpdateStatusOperation, Filename);

	// Get the SCC state
	FSourceControlStatePtr SourceControlState = SourceControlProvider.GetState(Filename, EStateCacheUsage::Use);

	if (SourceControlState.IsValid() && SourceControlState->IsSourceControlled()
		&& SourceControlState->GetHistorySize() > 0)
	{
		TSharedPtr<ISourceControlRevision, ESPMode::ThreadSafe> Revision = SourceControlState->GetHistoryItem(0);
		check(Revision.IsValid());

		FString TempFilename;
		if (Revision->Get(TempFilename))
		{
			OUU::Editor::JsonData::PerformDiff(
				FJsonDataAssetPath::FromPackagePath(
					OUU::Runtime::JsonData::SourceFullToPackage(TempFilename, EJsonDataAccessMode::Read)),
				FJsonDataAssetPath::FromPackagePath(
					OUU::Runtime::JsonData::SourceFullToPackage(Filename, EJsonDataAccessMode::Read)));
		}
	}
}

void FJsonFileSourceControlContextMenu::ExecuteSCCDiffAgainstDepot() const
{
	for (auto Asset : SelectedAssets)
	{
		DiffAgainstDepot(Asset->GetFilename());
	}
}

void FJsonFileSourceControlContextMenu::ExecuteSCCRevert()
{
	TArray<FString> PackageNames;
	GetSelectedPackageNames(PackageNames);
	FSourceControlWindows::PromptForRevert(PackageNames);
}

void FJsonFileSourceControlContextMenu::ExecuteSCCSync()
{
	TArray<FString> PackageNames;
	GetSelectedPackageNames(PackageNames);
	AssetViewUtils::SyncPackagesFromSourceControl(PackageNames);
}

void FJsonFileSourceControlContextMenu::GetSelectedPackageNames(TArray<FString>& OutPackageNames) const
{
	for (int32 AssetIdx = 0; AssetIdx < SelectedAssets.Num(); ++AssetIdx)
	{
		OutPackageNames.Add(SelectedAssets[AssetIdx]->GetFilename());
	}
}

#undef LOCTEXT_NAMESPACE
