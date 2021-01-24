// Copyright (c) 2021 Jonas Reich

#include "Misc/GeneralProjectSettingsLibrary.h"
#include "GeneralProjectSettings.h"

FString UGeneralProjectSettingsLibrary::GetProjectCompanyName()
{
	return GetDefault<UGeneralProjectSettings>()->CompanyName;
}

FString UGeneralProjectSettingsLibrary::GetProjectCompanyDistinguishedName()
{
	return GetDefault<UGeneralProjectSettings>()->CompanyDistinguishedName;
}

FString UGeneralProjectSettingsLibrary::GetProjectCopyrightNotice()
{
	return GetDefault<UGeneralProjectSettings>()->CopyrightNotice;
}

FString UGeneralProjectSettingsLibrary::GetProjectDescription()
{
	return GetDefault<UGeneralProjectSettings>()->Description;
}

FString UGeneralProjectSettingsLibrary::GetProjectHomepageUrl()
{
	return GetDefault<UGeneralProjectSettings>()->Homepage;
}

FString UGeneralProjectSettingsLibrary::GetProjectLicensingTerms()
{
	return GetDefault<UGeneralProjectSettings>()->LicensingTerms;
}

FString UGeneralProjectSettingsLibrary::GetProjectPrivacyPolicy()
{
	return GetDefault<UGeneralProjectSettings>()->PrivacyPolicy;
}

FGuid UGeneralProjectSettingsLibrary::GetProjectID()
{
	return GetDefault<UGeneralProjectSettings>()->ProjectID;
}

FString UGeneralProjectSettingsLibrary::GetProjectName()
{
	return GetDefault<UGeneralProjectSettings>()->ProjectName;
}

FString UGeneralProjectSettingsLibrary::GetProjectVersion()
{
	return GetDefault<UGeneralProjectSettings>()->ProjectVersion;
}

FString UGeneralProjectSettingsLibrary::GetProjectSupportContact()
{
	return GetDefault<UGeneralProjectSettings>()->SupportContact;
}
