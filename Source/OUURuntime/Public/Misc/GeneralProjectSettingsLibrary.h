// Copyright (c) 2021 Jonas Reich

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "GeneralProjectSettingsLibrary.generated.h"

/** Expose project info config values from UGeneralProjectSettings to Blueprint */
UCLASS()
class OUURUNTIME_API UGeneralProjectSettingsLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** @returns the name of the company (author, provider) that created the project. */
	UFUNCTION(BlueprintPure, Category = Publisher)
	static FString GetProjectCompanyName();

	/** @returns the distinguished name of the company (author, provider) that created the project. */
	UFUNCTION(BlueprintPure, Category = Publisher)
	static FString GetProjectCompanyDistinguishedName();

	/** @returns the project's copyright and/or trademark notices. */
	UFUNCTION(BlueprintPure, Category = Legal)
	static FString GetProjectCopyrightNotice();

	/** @returns the project's description text. */
	UFUNCTION(BlueprintPure, Category = About)
	static FString GetProjectDescription();

	/** @returns the project's homepage URL. */
	UFUNCTION(BlueprintPure, Category = Publisher)
	static FString GetProjectHomepageUrl();

	/** @returns the project's licensing terms. */
	UFUNCTION(BlueprintPure, Category = Legal)
	static FString GetProjectLicensingTerms();

	/** @returns the project's privacy policy. */
	UFUNCTION(BlueprintPure, Category = Legal)
	static FString GetProjectPrivacyPolicy();

	/** @returns the project's unique identifier. */
	UFUNCTION(BlueprintPure, Category = About)
	static FGuid GetProjectID();

	/** @returns the project's name. */
	UFUNCTION(BlueprintPure, Category = About)
	static FString GetProjectName();

	/** @returns the project's version number. */
	UFUNCTION(BlueprintPure, Category = About)
	static FString GetProjectVersion();

	/** @returns the project's support contact information. */
	UFUNCTION(BlueprintPure, Category = Publisher)
	static FString GetProjectSupportContact();
};
