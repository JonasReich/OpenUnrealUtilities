// Copyright (c) 2022 Jonas Reich

#pragma once

#include "CoreMinimal.h"

#include "Engine/DeveloperSettings.h"
#include "Engine/EngineTypes.h"

#include "OUUMapsToCookSettings.generated.h"

/** A list of maps for cooking in the ini file */
USTRUCT()
struct OUUDEVELOPER_API FOUUMapsToCookList
{
	GENERATED_BODY()
public:
	// Name of the config section that these properties reside in
	UPROPERTY(Transient, VisibleAnywhere)
	FString OwningConfigSection;

	UPROPERTY(EditAnywhere, meta = (RelativeToGameContentDir, LongPackageName))
	TArray<FFilePath> MapsToCook;

	void ReloadConfig(const FString& ConfigPath);
	void UpdateDefaultConfigFile(const FString& ConfigPath);
};

/**
 * Project settings that help managing "MapsToCook" lists in the DefaultEditor.ini file.
 * It's encouraged to use these settings exclusively to determine maps to cook and leave
 * UProjectPackagingSettings::MapsToCook empty.
 */
UCLASS(DefaultConfig, Config = Editor, PerObjectConfig, meta = (DisplayName = "Packaging - Maps to Cook (OUU)"))
class OUUDEVELOPER_API UOUUMapsToCookSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	UOUUMapsToCookSettings();

	// Request the package on default cooks.
	// Not used if commandline, AlwaysCookMaps, or MapsToCook are present.
	UPROPERTY(Transient, EditAnywhere, meta = (EditCondition = bEnableAllMaps))
	FOUUMapsToCookList AllMaps;

	// Request the package on every cook.
	// Using this prevents AllMaps.
	UPROPERTY(Transient, EditAnywhere)
	FOUUMapsToCookList AlwaysCookMaps;

	// Names of all custom config sections that contain map lists.
	UPROPERTY(Config, EditAnywhere, meta = (EditFixedOrder), DisplayName = "Custom Config Sections")
	TArray<FString> ConfigSections;

	// Map lists from ConfigSections that can be referenced used -MAPINISECTION parameter in command-line cooks
	UPROPERTY(
		Transient,
		EditAnywhere,
		EditFixedSize,
		meta = (EditFixedOrder),
		DisplayName = "Custom Config Sections Map Lists")
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

private:
#if WITH_EDITORONLY_DATA
	// Cache if AllMaps property should be editable
	UPROPERTY(Transient)
	bool bEnableAllMaps = true;

	// Temporary cache to compare edit changes before/after state
	// to detect renames.
	TArray<FString> ConfigSections_ChangeCopy;
#endif

	void RefreshMapListsFromConfig();
};
