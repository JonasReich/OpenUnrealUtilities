// Copyright (c) 2022 Jonas Reich

#include "Core/OUUCoreBlueprintLibrary.h"

#include "Engine/Engine.h"

UObject* UOUUCoreBlueprintLibrary::GetClassDefaultObject(TSubclassOf<UObject> Class)
{
	return Class != nullptr ? GetMutableDefault<UObject>(Class) : nullptr;
}

UObject* UOUUCoreBlueprintLibrary::GetClassDefaultObjectFromObject(UObject* Object)
{
	return IsValid(Object) ? GetMutableDefault<UObject>(Object->GetClass()) : nullptr;
}

UWorld* UOUUCoreBlueprintLibrary::TryGetWorldFromObject(UObject* WorldContextObject)
{
	return GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
}

UWorld* UOUUCoreBlueprintLibrary::TryGetWorldFromObject_K2(UObject* WorldContextObject)
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

FString UOUUCoreBlueprintLibrary::Conv_TopLevelAssetPathToString(FTopLevelAssetPath InPath)
{
	return InPath.ToString();
}

FTopLevelAssetPath UOUUCoreBlueprintLibrary::Conv_StringToTopLevelAssetPath(FString InPath)
{
	return FTopLevelAssetPath(InPath);
}

FTopLevelAssetPath UOUUCoreBlueprintLibrary::Conv_ClassToTopLevelAssetPath(UClass* InClass)
{
	return IsValid(InClass) ? FTopLevelAssetPath(InClass->GetClassPathName()) : FTopLevelAssetPath{};
}
