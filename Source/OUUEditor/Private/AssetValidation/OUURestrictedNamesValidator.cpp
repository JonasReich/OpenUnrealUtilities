// Copyright (c) 2025 Jonas Reich & Contributors

#include "AssetValidation/OUURestrictedNamesValidator.h"

bool UOUURestrictedNamesValidator::CanValidateAsset_Implementation(
	const FAssetData& InAssetData,
	UObject* InObject,
	FDataValidationContext& InContext) const
{
	if (InObject->IsA<UObjectRedirector>())
	{
		return false;
	}

	return true;
}

EDataValidationResult UOUURestrictedNamesValidator::ValidateLoadedAsset_Implementation(
	const FAssetData& InAssetData,
	UObject* InAsset,
	FDataValidationContext& Context)
{
	// Use the package name instead of package path to make sure every folder starts and ends in a slash to simplify
	// checks below.
	const auto PackagePath = InAssetData.PackageName.ToString();
	if (PackagePath.StartsWith(TEXT("/Game/")) == false)
	{
		// Never validate content outside the main project content dir
		return EDataValidationResult::Valid;
	}

	bool IsRestrictedPath = FPaths::IsRestrictedPath(PackagePath);
	if (IsRestrictedPath == false)
	{
		// The C++ restricted path list (in 5.2) doesn't contain all the names that are checked by UAT / packaging code
		// in C#. It only adds "active" confidential platforms and internal dirs like "NotForLicensees". So we need to
		// check for some additional restricted keywords (especially ones that easily cause conflicts with props):
		IsRestrictedPath |= PackagePath.Contains(TEXT("/Apple/"));
		IsRestrictedPath |= PackagePath.Contains(TEXT("/Desktop/"));
		IsRestrictedPath |= PackagePath.Contains(TEXT("/Switch/"));
		IsRestrictedPath |= PackagePath.Contains(TEXT("/Windows/"));
	}

	if (IsRestrictedPath)
	{
		if (FApp::IsEngineInstalled() == false)
		{
			// check for allowed directories if the engine is not installed
			FString LocalPath;
			FPackageName::TryConvertLongPackageNameToFilename(PackagePath, LocalPath);
			FPaths::MakePathRelativeTo(LocalPath, *FPaths::RootDir());
			// these are exceptions that contain platform specific content
			TArray<FString> AllowedDirectories;
			GConfig->GetArray(TEXT("Staging"), TEXT("AllowedDirectories"), OUT AllowedDirectories, GGameIni);
			// these are known platform specific folders
			TArray<FString> AllowedPlatformDirectories;
			GConfig->GetArray(
				TEXT("Staging"),
				TEXT("KnownPlatformDirectories"),
				OUT AllowedPlatformDirectories,
				GGameIni);
			AllowedDirectories.Append(AllowedPlatformDirectories);
			for (auto& Dir : AllowedDirectories)
			{
				if (LocalPath.StartsWith(Dir))
				{
					return EDataValidationResult::Valid;
				}
			}
		}

		Context.AddError(INVTEXT("Asset is in a restricted platform specific directory (e.g. Windows/XSX/Apple/etc). "
								 "Please rename or add an exception to Game.ini [Staging]AllowedDirectories if this is "
								 "NOT a platform directory or [Staging]KnownPlatformDirectories if it IS."));
		return EDataValidationResult::Invalid;
	}

	return EDataValidationResult::Valid;
}
