// Copyright (c) 2023 Jonas Reich & Contributors

#include "OUUMapsToCookSettings.h"

#include "GameMapsSettings.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "HAL/PlatformFileManager.h"
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

void FOUUMapsToCookList::ReloadConfig(const FString& ConfigPath)
{
	TArray<FString> MapsToCookStrings;
	if (GConfig->GetArray(
			*OwningConfigSection,
			TEXT("Map"), // Use hardcoded key name "Map" as required by the cooker
			MapsToCookStrings,
			ConfigPath))
	{
		MapsToCook.Empty();
		for (auto& Map : MapsToCookStrings)
		{
			MapsToCook.Add_GetRef(FFilePath()).FilePath = Map;
		}
	}
}

void FOUUMapsToCookList::UpdateDefaultConfigFile(const FString& ConfigPath)
{
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

UOUUMapsToCookSettings::UOUUMapsToCookSettings()
{
	AllMaps.OwningConfigSection = TEXT("AllMaps");
	AlwaysCookMaps.OwningConfigSection = TEXT("AlwaysCookMaps");
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
		AllMaps.UpdateDefaultConfigFile(ConfigPath);
		AlwaysCookMaps.UpdateDefaultConfigFile(ConfigPath);
#if WITH_EDITORONLY_DATA
		bEnableAllMaps = AlwaysCookMaps.MapsToCook.Num() == 0;
#endif
	}

	OUU::Developer::Private::CheckoutConfigFile(ConfigPath);
	GConfig->Flush(false, ConfigPath);
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

void UOUUMapsToCookSettings::RefreshMapListsFromConfig()
{
	AllMaps.ReloadConfig(GEditorIni);
	AlwaysCookMaps.ReloadConfig(GEditorIni);

	MapLists.Empty();
	for (auto& ConfigSectionName : ConfigSections)
	{
		auto& MapsToCookWrapperObj = MapLists.AddDefaulted_GetRef();
		MapsToCookWrapperObj.OwningConfigSection = ConfigSectionName;

		// For some reason this needs the Saved/ config path and cannot handle the Default config path
		MapsToCookWrapperObj.ReloadConfig(GEditorIni);
	}

#if WITH_EDITORONLY_DATA
	bEnableAllMaps = AlwaysCookMaps.MapsToCook.Num() == 0;
#endif
}
