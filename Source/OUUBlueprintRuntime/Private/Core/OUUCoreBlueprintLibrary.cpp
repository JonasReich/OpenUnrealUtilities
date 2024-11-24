// Copyright (c) 2023 Jonas Reich & Contributors

#include "Core/OUUCoreBlueprintLibrary.h"

#include "Engine/Engine.h"

UObject* UOUUCoreBlueprintLibrary::GetClassDefaultObject(TSubclassOf<UObject> Class)
{
	return Class != nullptr ? GetMutableDefault<UObject>(Class) : nullptr;
}

UObject* UOUUCoreBlueprintLibrary::GetClassDefaultObjectFromObject(const UObject* Object)
{
	return IsValid(Object) ? GetMutableDefault<UObject>(Object->GetClass()) : nullptr;
}

UWorld* UOUUCoreBlueprintLibrary::TryGetWorldFromObject(const UObject* WorldContextObject)
{
	return GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
}

UWorld* UOUUCoreBlueprintLibrary::TryGetWorldFromObject_K2(const UObject* WorldContextObject)
{
	return TryGetWorldFromObject(WorldContextObject);
}

void UOUUCoreBlueprintLibrary::ModifyObject(UObject* Object)
{
	if (IsValid(Object))
	{
		Object->Modify();
	}
}

FString UOUUCoreBlueprintLibrary::Conv_TopLevelAssetPathToString(const FTopLevelAssetPath& InPath)
{
	return InPath.ToString();
}

FTopLevelAssetPath UOUUCoreBlueprintLibrary::Conv_StringToTopLevelAssetPath(const FString& InPath)
{
	return FTopLevelAssetPath(InPath);
}

FTopLevelAssetPath UOUUCoreBlueprintLibrary::Conv_ClassToTopLevelAssetPath(const UClass* InClass)
{
	return IsValid(InClass) ? FTopLevelAssetPath(InClass->GetClassPathName()) : FTopLevelAssetPath{};
}
