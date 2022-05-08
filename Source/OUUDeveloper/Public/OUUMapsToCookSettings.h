// Copyright (c) 2022 Jonas Reich

#pragma once

#include "CoreMinimal.h"

#include "Engine/DeveloperSettings.h"
#include "Engine/EngineTypes.h"

#include "OUUMapsToCookSettings.generated.h"

/** A list of maps for cooking */
USTRUCT()
struct OUUDEVELOPER_API FOUUMapsToCookList
{
	GENERATED_BODY()
public:
	// Name of the config section that these properties reside in
	UPROPERTY(Transient, VisibleAnywhere)
	FString OwningConfigSection;

	/**
	 * The map that will be loaded by default when no other map is loaded.
	 *
	 * This value will be copied over during cook or when changing the property via editor to its
	 * native counterpart in UGameMapsSettings.
	 *
	 * If you have multiple active config sections for maps, only the value from the
	 * first active ini section will be used.
	 */
	UPROPERTY(EditAnywhere, meta = (AllowedClasses = "World"))
	FSoftObjectPath GameDefaultMap;

	UPROPERTY(EditAnywhere, meta = (RelativeToGameContentDir, LongPackageName))
	TArray<FFilePath> MapsToCook;

	void ReloadConfig();
	void UpdateDefaultConfigFile(FString ConfigPath);
};

/**
 * Project settings that help managing "MapsToCook" lists in the DefaultGame.ini file.
 * This is a replacement for UProjectPackagingSettings::MapsToCook.
 *
 * Even if you do not use the DefaultConfigSection setting,
 * it's still recommended to use these settings to manage your map lists.
 */
UCLASS(DefaultConfig, Config = Game, PerObjectConfig, meta = (DisplayName = "Packaging - Maps to Cook (OUU)"))
class OUUDEVELOPER_API UOUUMapsToCookSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	/**
	 * Select a valid config section from the list of available config sections below.
	 * When this field is set to a non-empty string that is contained within the list below
	 * it's used as default config section for the "maps to cook" list.
	 *
	 * Order of precedence:
	 *     1. MAPINISECTION cli parameter
	 *         -> changes cook maps only
	 *         -> first maps set will be used for default map if possible
	 *     2. [/Script/OUUDeveloper.OUUMapsToCookSettings] DefaultConfigSection (this ini variable)
	 *         -> changes cook maps & default map
	 *     3. [/Script/UnrealEd.ProjectPackagingSettings] MapsToCook
	 *         -> changes cook maps only
	 *
	 * Please note that with both command line parameters you also have the option to combine map lists,
	 * which this field does not allow (yet) because of GUI limitations.
	 */
	UPROPERTY(Config, EditAnywhere)
	FString DefaultConfigSection;

	// Names of all available config sections that contain map lists.
	UPROPERTY(Config, EditAnywhere, meta = (EditFixedOrder))
	TArray<FString> ConfigSections;

	// Display/edit wrappers for the ini sections that contain map lists.
	UPROPERTY(Transient, EditAnywhere, EditFixedSize, meta = (EditFixedOrder))
	TArray<FOUUMapsToCookList> MapLists;

	// - UObject
	virtual void PostReloadConfig(FProperty* PropertyThatWasLoaded) override;
#if WITH_EDITOR
	virtual void PreEditChange(FProperty* PropertyAboutToChange) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	virtual void PostInitProperties() override;
	// --

	virtual FName GetCategoryName() const override;

	/**
	 * Try to inject a MAPINISECTION parameter into the commandline based on these settings.
	 * If one is already present, it's used as basis to set the default game map if possible.
	 *
	 * Also applies the GameDefaultMap of the first active map ini section.
	 */
	static void TryInjectMapIniSectionCommandlineForCook();

private:
#if WITH_EDITOR
	// Temporary cache to compare edit changes before/after state
	TArray<FString> ConfigSections_ChangeCopy;
#endif

	void SetDefaultConfigSection(FString ConfigSectionString);

	void RefreshMapListsFromConfig();

	void TryApplyGameDefaultMap() const;
};
