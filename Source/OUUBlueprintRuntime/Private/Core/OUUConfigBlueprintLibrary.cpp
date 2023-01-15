// Copyright (c) 2022 Jonas Reich

#include "Core/OUUConfigBlueprintLibrary.h"

#include "Engine.h"

FString UOUUConfigBlueprintLibrary::GetConfigIniPath(EGlobalIniFile IniFile)
{
#define SWITCH_CASE_INI(ConfigName)                                                                                    \
	case EGlobalIniFile::ConfigName: return G##ConfigName##Ini;

	switch (IniFile)
	{
		SWITCH_CASE_INI(Engine)
		SWITCH_CASE_INI(Editor)
		SWITCH_CASE_INI(EditorKeyBindings)
		SWITCH_CASE_INI(EditorLayout)
		SWITCH_CASE_INI(EditorSettings)
		SWITCH_CASE_INI(EditorPerProject)
		SWITCH_CASE_INI(Compat)
		SWITCH_CASE_INI(Lightmass)
		SWITCH_CASE_INI(Scalability)
		SWITCH_CASE_INI(Hardware)
		SWITCH_CASE_INI(Input)
		SWITCH_CASE_INI(Game)
		SWITCH_CASE_INI(GameUserSettings)
		SWITCH_CASE_INI(RuntimeOptions)
		SWITCH_CASE_INI(InstallBundle)
		SWITCH_CASE_INI(DeviceProfiles)
		SWITCH_CASE_INI(GameplayTags)

	default: checkf(false, TEXT("Invalid IniFile entry")); return "";
	}

#undef SWITCH_CASE_INI
}

FString UOUUConfigBlueprintLibrary::GetConfigString(
	const FString& Section,
	const FString& Key,
	const FString& IniFilename)
{
	if (!GConfig)
	{
		return "";
	}
	return GConfig->GetStr(*Section, *Key, IniFilename);
}

#define OUU_BP_GET_CONFIG_VALUE(What, ReturnValue)                                                                     \
	if (!GConfig)                                                                                                      \
	{                                                                                                                  \
		Value = ReturnValue;                                                                                           \
		return false;                                                                                                  \
	}                                                                                                                  \
	return GConfig->Get##What(*Section, *Key, Value, IniFilename);

bool UOUUConfigBlueprintLibrary::GetConfigInt(
	const FString& Section,
	const FString& Key,
	int32& Value,
	const FString& IniFilename)
{
	OUU_BP_GET_CONFIG_VALUE(Int, 0);
}

bool UOUUConfigBlueprintLibrary::GetConfigFloat(
	const FString& Section,
	const FString& Key,
	float& Value,
	const FString& IniFilename)
{
	OUU_BP_GET_CONFIG_VALUE(Float, 0.0f);
}

bool UOUUConfigBlueprintLibrary::GetConfigDouble(
	const FString& Section,
	const FString& Key,
	double& Value,
	const FString& IniFilename)
{
	OUU_BP_GET_CONFIG_VALUE(Double, 0.0);
}

bool UOUUConfigBlueprintLibrary::GetConfigBool(
	const FString& Section,
	const FString& Key,
	bool& Value,
	const FString& IniFilename)
{
	OUU_BP_GET_CONFIG_VALUE(Bool, false);
}

bool UOUUConfigBlueprintLibrary::GetConfigArray(
	const FString& Section,
	const FString& Key,
	TArray<FString>& Value,
	const FString& IniFilename)
{
	if (!GConfig)
	{
		Value.Empty();
		return false;
	}
	const int32 ArrayCount = GConfig->GetArray(*Section, *Key, Value, IniFilename);
	return ArrayCount > 0;
}

bool UOUUConfigBlueprintLibrary::GetConfigColor(
	const FString& Section,
	const FString& Key,
	FColor& Value,
	const FString& IniFilename)
{
	OUU_BP_GET_CONFIG_VALUE(Color, FColor::Black);
}

bool UOUUConfigBlueprintLibrary::GetConfigVector2D(
	const FString& Section,
	const FString& Key,
	FVector2D& Value,
	const FString& IniFilename)
{
	OUU_BP_GET_CONFIG_VALUE(Vector2D, FVector2D::Zero());
}

bool UOUUConfigBlueprintLibrary::GetConfigVector(
	const FString& Section,
	const FString& Key,
	FVector& Value,
	const FString& IniFilename)
{
	OUU_BP_GET_CONFIG_VALUE(Vector, FVector::Zero());
}

bool UOUUConfigBlueprintLibrary::GetConfigVector4(
	const FString& Section,
	const FString& Key,
	FVector4& Value,
	const FString& IniFilename)
{
	OUU_BP_GET_CONFIG_VALUE(Vector4, FVector4::Zero());
}

bool UOUUConfigBlueprintLibrary::GetConfigRotator(
	const FString& Section,
	const FString& Key,
	FRotator& Value,
	const FString& IniFilename)
{
	OUU_BP_GET_CONFIG_VALUE(Rotator, FRotator::ZeroRotator);
}

#undef OUU_BP_GET_CONFIG_VALUE
