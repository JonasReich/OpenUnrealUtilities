// Copyright (c) 2023 Jonas Reich & Contributors

#include "Commandlets/OUUValidateAssetListCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Dom/JsonObject.h"
#include "EditorValidatorSubsystem.h"
#include "LogOpenUnrealUtilities.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonSerializer.h"
#include "Editor.h"

int32 UOUUValidateAssetListCommandlet::Main(const FString& FullCommandLine)
{
	FString AssetListFilePath;
	FParse::Value(*FullCommandLine, TEXT("AssetList="), AssetListFilePath);
	FString ValidationReportPath;
	FParse::Value(*FullCommandLine, TEXT("ValidationReport="), ValidationReportPath);

	TArray<FAssetData> AssetDataList;

	FString AssetListString;
	if (FFileHelper::LoadFileToString(AssetListString, *AssetListFilePath) == false)
	{
		UE_LOG(LogOpenUnrealUtilities, Error, TEXT("Failed to read asset list %s"), *AssetListFilePath);
		return -1;
	}

	UEditorValidatorSubsystem* EditorValidationSubsystem = GEditor->GetEditorSubsystem<UEditorValidatorSubsystem>();
	check(EditorValidationSubsystem);

	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(FName("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	auto ReportObject = MakeShared<FJsonObject>();

	int32 NumInvalidAssets = 0;

	const TCHAR* AssetListBuffer = *AssetListString;
	FString PackageName;
	while (FParse::Line(&AssetListBuffer, PackageName))
	{
		TArray<FAssetData> PackageAssetsData;
		AssetRegistry.GetAssetsByPackageName(*PackageName, OUT PackageAssetsData);

		TArray<FText> PackageErrors, PackageWarnings;
		EDataValidationResult PackageResult = EDataValidationResult::NotValidated;
		for (auto& Asset : PackageAssetsData)
		{
			auto AssetResult =
				EditorValidationSubsystem
					->IsAssetValid(Asset, OUT PackageErrors, OUT PackageWarnings, EDataValidationUsecase::Commandlet);

			PackageResult = CombineDataValidationResults(AssetResult, PackageResult);
		}
		if (PackageResult == EDataValidationResult::Invalid)
		{
			TArray<FString> PackageErrorStrings;
			for (auto& PackageError : PackageErrors)
			{
				PackageErrorStrings.Add(PackageError.ToString());
			}

			FString PackageErrorString = FString::Join(PackageErrorStrings, LINE_TERMINATOR);
			ReportObject->SetStringField(PackageName, PackageErrorString);

			++NumInvalidAssets;
		}
	}

	FString JsonString;
	const TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(OUT & JsonString);
	ensure(FJsonSerializer::Serialize(ReportObject, JsonWriter));
	ensure(FFileHelper::SaveStringToFile(JsonString, *ValidationReportPath));

	return NumInvalidAssets;
}
