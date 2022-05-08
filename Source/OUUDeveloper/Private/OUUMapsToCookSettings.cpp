// Copyright (c) 2022 Jonas Reich

#include "OUUMapsToCookSettings.h"

#include "GameMapsSettings.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "HAL/PlatformFilemanager.h"
#include "ISourceControlModule.h"
#include "LogOpenUnrealUtilities.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "SourceControlHelpers.h"

namespace OUU::Developer::Private
{
	void CheckoutConfigFile(const FString& RelativeConfigFilePath)
	{
		FString ConfigPath = FPaths::ConvertRelativePathToFull(RelativeConfigFilePath);

		if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*ConfigPath))
		{
			return;
		}

		if (ISourceControlModule::Get().IsEnabled())
		{
			FText ErrorMessage;

			if (!SourceControlHelpers::CheckoutOrMarkForAdd(
					ConfigPath,
					FText::FromString(ConfigPath),
					NULL,
					ErrorMessage))
			{
				UE_LOG(LogOpenUnrealUtilities, Error, TEXT("%s"), *ErrorMessage.ToString());
			}
		}
		else
		{
			if (!FPlatformFileManager::Get().GetPlatformFile().SetReadOnly(*ConfigPath, false))
			{
				UE_LOG(LogOpenUnrealUtilities, Error, TEXT("Could not make %s writable."), *ConfigPath);
			}
		}
	}
} // namespace OUU::Developer::Private

void FOUUMapsToCookList::ReloadConfig()
{
	FString GameDefaultMapValue;
	if (GConfig->GetString(
			*OwningConfigSection,
			GET_MEMBER_NAME_STRING_CHECKED(FOUUMapsToCookList, GameDefaultMap),
			GameDefaultMapValue,
			GGameIni))
	{
		GameDefaultMap = GameDefaultMapValue;
	}

	TArray<FString> MapsToCookStrings;
	if (GConfig->GetArray(
			*OwningConfigSection,
			TEXT("Map"), // Use hardcoded key name "Map" as required by the cooker
			MapsToCookStrings,
			GGameIni))
	{
		MapsToCook.Empty();
		for (auto& Map : MapsToCookStrings)
		{
			MapsToCook.Add_GetRef(FFilePath()).FilePath = Map;
		}
	}
}

void FOUUMapsToCookList::UpdateDefaultConfigFile(FString ConfigPath)
{
	if (GameDefaultMap.IsValid())
	{
		GConfig->SetString(
			*OwningConfigSection,
			GET_MEMBER_NAME_STRING_CHECKED(FOUUMapsToCookList, GameDefaultMap),
			*GameDefaultMap.ToString(),
			ConfigPath);
	}
	else
	{
		GConfig->RemoveKey(
			*OwningConfigSection,
			GET_MEMBER_NAME_STRING_CHECKED(FOUUMapsToCookList, GameDefaultMap),
			ConfigPath);
	}

	// Default ini's require the array syntax to be applied to the property name
	// We also use the hardcoded name "Map", because that's required by the cooker.
	const FString CompleteKey = TEXT("+Map");
	FConfigSection* Sec = GConfig->GetSectionPrivate(*OwningConfigSection, true, false, *ConfigPath);
	if (MapsToCook.Num() > 0)
	{
		TArray<FString> MapsToCookStrings;
		for (auto& Map : MapsToCook)
		{
			MapsToCookStrings.Add(Map.FilePath);
		}

		if (Sec)
		{
			// Delete the old value for the property in the ConfigCache before (conditionally) adding in the new value
			Sec->Remove(*CompleteKey);
		}

		for (int32 i = 0; i < MapsToCook.Num(); i++)
		{
			Sec->Add(*CompleteKey, *MapsToCook[i].FilePath);
		}
	}
	else
	{
		Sec->Remove(*CompleteKey);
	}
}

void UOUUMapsToCookSettings::PostReloadConfig(FProperty* PropertyThatWasLoaded)
{
	Super::PostReloadConfig(PropertyThatWasLoaded);
	RefreshMapListsFromConfig();
}

#if WITH_EDITOR
void UOUUMapsToCookSettings::PreEditChange(FProperty* PropertyAboutToChange)
{
	Super::PreEditChange(PropertyAboutToChange);
	if (PropertyAboutToChange
		&& PropertyAboutToChange->GetFName() == GET_MEMBER_NAME_CHECKED(UOUUMapsToCookSettings, ConfigSections))
	{
		ConfigSections_ChangeCopy = CopyTemp(ConfigSections);
	}
}

void UOUUMapsToCookSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	auto ConfigPath = GetDefaultConfigFilename();

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UOUUMapsToCookSettings, ConfigSections))
	{
		if (ConfigSections.Num() == ConfigSections_ChangeCopy.Num())
		{
			// case #1: Rename

			for (int32 SectionIdx = 0; SectionIdx < ConfigSections_ChangeCopy.Num(); SectionIdx++)
			{
				auto& OldSectionName = ConfigSections_ChangeCopy[SectionIdx];
				auto& NewSectionName = ConfigSections[SectionIdx];

				if (OldSectionName == NewSectionName)
					continue;

				// Remove old section
				GConfig->EmptySection(*OldSectionName, *ConfigPath);

				// Rename wrapper
				auto& Wrapper = MapLists[SectionIdx];
				Wrapper.OwningConfigSection = NewSectionName;

				// export from renamed section
				Wrapper.UpdateDefaultConfigFile(GetDefaultConfigFilename());
			}
		}
		else if (ConfigSections.Num() < ConfigSections_ChangeCopy.Num())
		{
			// case #2: Delete
			for (int32 SectionIdx = 0; SectionIdx < ConfigSections_ChangeCopy.Num(); SectionIdx++)
			{
				// Remove old keys that are not in new list
				auto& OldSectionName = ConfigSections_ChangeCopy[SectionIdx];
				if (ConfigSections.Find(OldSectionName))
					continue;

				GConfig->EmptySection(*OldSectionName, *ConfigPath);
			}
		}
		else
		{
			// case #3: Add
			RefreshMapListsFromConfig();
		}
	}
	else
	{
		for (auto& NestedSetting : MapLists)
		{
			NestedSetting.UpdateDefaultConfigFile(ConfigPath);
		}
	}

	OUU::Developer::Private::CheckoutConfigFile(ConfigPath);
	GConfig->Flush(false, ConfigPath);
	TryApplyGameDefaultMap();
}
#endif

void UOUUMapsToCookSettings::PostInitProperties()
{
	Super::PostInitProperties();
	RefreshMapListsFromConfig();
}

FName UOUUMapsToCookSettings::GetCategoryName() const
{
	return TEXT("Project");
}

void UOUUMapsToCookSettings::TryInjectMapIniSectionCommandlineForCook()
{
	UE_LOG(LogOpenUnrealUtilities, Log, TEXT("Checking for cook..."));

	if (!IsRunningCommandlet())
	{
		UE_LOG(LogOpenUnrealUtilities, Log, TEXT("No cook command detected (not running commandlet)."));
		return;
	}

	FString CommandLine = FCommandLine::Get();

	const bool bFoundCommandletName =
		CommandLine.Contains(TEXT("cookcommandlet")) || CommandLine.Contains(TEXT("run=cook"));
	if (!bFoundCommandletName)
	{
		UE_LOG(LogOpenUnrealUtilities, Log, TEXT("No cook command detected (running different commandlet)."));
		return;
	}

	UE_LOG(
		LogOpenUnrealUtilities,
		Log,
		TEXT("Cook command detected. "
			 "Attempting to modify command line based on UOUUMapsToCookSettings..."));

	auto* MapsToCookSettings = GetMutableDefault<UOUUMapsToCookSettings>();
	if (!IsValid(MapsToCookSettings))
		return;

	// do not modify the commandline if MAPINISECTION parameter was set
	FString CombinedSectionStr;
	const bool bHasNativeIniSectionCliArg = FParse::Value(*CommandLine, TEXT("MAPINISECTION="), OUT CombinedSectionStr);
	if (!bHasNativeIniSectionCliArg)
	{
		CombinedSectionStr = MapsToCookSettings->DefaultConfigSection;
	}

	TArray<FString> MapIniSections;
	if (CombinedSectionStr.Contains(TEXT("+")))
	{
		TArray<FString> Sections;
		CombinedSectionStr.ParseIntoArray(Sections, TEXT("+"), true);
		for (int32 Index = 0; Index < Sections.Num(); Index++)
		{
			MapIniSections.Add(Sections[Index]);
		}
	}
	else
	{
		MapIniSections.Add(CombinedSectionStr);
	}

	if (MapIniSections.Num() <= 0)
	{
		UE_LOG(LogOpenUnrealUtilities, Log, TEXT("No map ini sections found. Cooking with default map settings."));
		return;
	}

	const auto& FirstMapSection = MapIniSections[0];
	if (!GConfig->DoesSectionExist(*FirstMapSection, GGameIni))
	{
		UE_LOG(LogOpenUnrealUtilities, Warning, TEXT("Map ini section '%s' does not exist"), *FirstMapSection);
	}

	GetMutableDefault<UOUUMapsToCookSettings>()->SetDefaultConfigSection(FirstMapSection);
	auto* GameMapSettings = GetDefault<UGameMapsSettings>();
	if (GameMapSettings->GetGameDefaultMap().IsEmpty())
	{
		UE_LOG(LogOpenUnrealUtilities, Fatal, TEXT("Game default map is not set"));
		FPlatformMisc::RequestExitWithStatus(false, 1);
	}

	if (!bHasNativeIniSectionCliArg)
	{
		// append custom MAPINISECTION parameter at the end of original commandline
		const FString NewArgument = FString::Printf(TEXT("-MAPINISECTION=%s"), *CombinedSectionStr);
		FCommandLine::Set(*FString::Printf(TEXT("%s %s"), *CommandLine, *NewArgument));

		UE_LOG(
			LogOpenUnrealUtilities,
			Log,
			TEXT("Appended '%s' to the command line. "
				 "This should cause the cooker to use all '+Map=' entries "
				 "in section [%s] from DefaultGame.ini as map list."),
			*NewArgument,
			*MapsToCookSettings->DefaultConfigSection);
	}
}

void UOUUMapsToCookSettings::SetDefaultConfigSection(FString ConfigSectionString)
{
	DefaultConfigSection = ConfigSectionString;

	// Write out the changed DefaultConfigSection
	OUU::Developer::Private::CheckoutConfigFile(GetDefaultConfigFilename());
	UpdateDefaultConfigFile();

	TryApplyGameDefaultMap();
}

void UOUUMapsToCookSettings::RefreshMapListsFromConfig()
{
	MapLists.Empty();
	for (auto& ConfigSectionName : ConfigSections)
	{
		auto& MapsToCookWrapperObj = MapLists.AddDefaulted_GetRef();
		MapsToCookWrapperObj.OwningConfigSection = ConfigSectionName;
		MapsToCookWrapperObj.ReloadConfig();
	}
}

void UOUUMapsToCookSettings::TryApplyGameDefaultMap() const
{
	auto* GameMapSettings = GetMutableDefault<UGameMapsSettings>();
	FString GameDefaultMap;
	if (GConfig->GetString(
			*DefaultConfigSection,
			*FString(GET_MEMBER_NAME_STRING_CHECKED(FOUUMapsToCookList, GameDefaultMap)),
			OUT GameDefaultMap,
			GGameIni))
	{
		UE_LOG(
			LogOpenUnrealUtilities,
			Log,
			TEXT("Chaning game default map to %s because it's set in ini section [%s] for MapsToCook."),
			*GameDefaultMap,
			*DefaultConfigSection);

		GameMapSettings->SetGameDefaultMap(GameDefaultMap);

		// Write out the changed default game map, so it's applied for the build.
		OUU::Developer::Private::CheckoutConfigFile(GameMapSettings->GetDefaultConfigFilename());
		GameMapSettings->UpdateDefaultConfigFile();
	}
}
