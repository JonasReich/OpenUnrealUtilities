// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "Commandlets/Commandlet.h"

#include "OUUValidateAssetListCommandlet.generated.h"

class IAssetRegistry;
class UEditorValidatorSubsystem;

UCLASS()
class OUUEDITOR_API UOUUValidateAssetListCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	// - UCommandlet
	int32 Main(const FString& FullCommandLine) override;
	// --

private:
	void ValidateEntry(
		const UEditorValidatorSubsystem& EditorValidationSubsystem,
		const IAssetRegistry& AssetRegistry,
		const FString& AssetListEntry,
		const TSharedRef<FJsonObject>& OutJsonObject,
		int32& OutNumInvalidAssets);

	void ValidateSingleAsset(
		const UEditorValidatorSubsystem& EditorValidationSubsystem,
		const TSharedRef<IMessageLogListing>& MapCheckListing,
		const FAssetData& Asset,
		const TSharedRef<FJsonObject>& OutReportObject,
		int32& OutNumInvalidAssets);
};
