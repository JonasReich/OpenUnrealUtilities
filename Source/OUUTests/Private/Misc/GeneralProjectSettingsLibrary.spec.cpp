// Copyright (c) 2021 Jonas Reich

#include "OUUTests.h"

#if WITH_AUTOMATION_WORKER

#include "Misc/GeneralProjectSettingsLibrary.h"

BEGIN_DEFINE_SPEC(FGeneralProjectSettingsLibrarySpec, "OpenUnrealUtilities.Misc.GeneralProjectSettingsLibrary", DEFAULT_OUU_TEST_FLAGS)
END_DEFINE_SPEC(FGeneralProjectSettingsLibrarySpec)
void FGeneralProjectSettingsLibrarySpec::Define()
{
	It("should return a valid GUID as project ID", [this]()
	{
		FGuid ProjectId = UGeneralProjectSettingsLibrary::GetProjectID();
		SPEC_TEST_TRUE(ProjectId.IsValid());
	});

	It("should return a non-empty string as project version", [this]()
	{
		FString ProjectVersion = UGeneralProjectSettingsLibrary::GetProjectVersion();
		SPEC_TEST_TRUE(ProjectVersion.Len() > 0);
	});

	It("should return the same project version that is set in the ini", [this]()
	{
		FString ProjectVersionFromLibrary = UGeneralProjectSettingsLibrary::GetProjectVersion();
		FString ProjectVersionFromIni;
		bool bValueWasFoundInIni = GConfig->GetString(
			TEXT("/Script/EngineSettings.GeneralProjectSettings"),
			TEXT("ProjectVersion"),
			ProjectVersionFromIni,
			GGameIni
		);

		SPEC_TEST_TRUE(bValueWasFoundInIni);
		SPEC_TEST_EQUAL(ProjectVersionFromLibrary, ProjectVersionFromIni);
	});

	// There is no way to validate if any of the other settings are correctly set,
	// because they are all empty by default and many of them might stay empty permanently.
	// Checking for specific values would also be counter-productive as these tests are contained in a plugin
	// that should be redistributable separately from the project.
}

#endif
