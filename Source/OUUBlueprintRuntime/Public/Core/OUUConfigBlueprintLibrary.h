// Copyright (c) 2022 Jonas Reich

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Templates/SubclassOf.h"

#include "OUUConfigBlueprintLibrary.generated.h"

/** All global ini files declared in CoreGlobals.h */
UENUM(BlueprintType)
enum class EGlobalIniFile : uint8
{
	Engine,
	Editor,
	EditorKeyBindings,
	EditorLayout,
	EditorSettings,
	EditorPerProject,
	Compat,
	Lightmass,
	Scalability,
	Hardware,
	Input,
	Game,
	GameUserSettings,
	RuntimeOptions,
	InstallBundle,
	DeviceProfiles,
	GameplayTags
};

/**
 * Blueprint wrapper for GConfig.
 * With these functions, you can access ini config data.
 */
UCLASS()
class UOUUConfigBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** Get the file path to a global config file usable for GetConfigX functions (e.g. GetConfigString). */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Config")
	static FString GetConfigIniPath(EGlobalIniFile IniFile);

	/**
	 * Get the string value for a config entry.
	 * This call cannot fail and will return an empty string if the Section/Key are not valid.
	 */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Config")
	static FString GetConfigString(const FString& Section, const FString& Key, const FString& IniFilename);

	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Config")
	static bool GetConfigInt(const FString& Section, const FString& Key, int32& Value, const FString& IniFilename);
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Config")
	static bool GetConfigFloat(const FString& Section, const FString& Key, float& Value, const FString& IniFilename);
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Config")
	static bool GetConfigDouble(const FString& Section, const FString& Key, double& Value, const FString& IniFilename);
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Config")
	static bool GetConfigBool(const FString& Section, const FString& Key, bool& Value, const FString& IniFilename);
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Config")
	static bool GetConfigArray(
		const FString& Section,
		const FString& Key,
		TArray<FString>& Value,
		const FString& IniFilename);
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Config")
	static bool GetConfigColor(const FString& Section, const FString& Key, FColor& Value, const FString& IniFilename);
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Config")
	static bool GetConfigVector2D(
		const FString& Section,
		const FString& Key,
		FVector2D& Value,
		const FString& IniFilename);
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Config")
	static bool GetConfigVector(const FString& Section, const FString& Key, FVector& Value, const FString& IniFilename);
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Config")
	static bool GetConfigVector4(
		const FString& Section,
		const FString& Key,
		FVector4& Value,
		const FString& IniFilename);
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Config")
	static bool GetConfigRotator(
		const FString& Section,
		const FString& Key,
		FRotator& Value,
		const FString& IniFilename);
};