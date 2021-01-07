// Copyright (c) 2021 Jonas Reich

#include "Core/CoreBlueprintLibrary.h"

UObject* UCoreBlueprintLibrary::GetClassDefaultObject(TSubclassOf<UObject> Class)
{
	return Class != nullptr ? GetMutableDefault<UObject>(Class) : nullptr;
}

UObject* UCoreBlueprintLibrary::GetClassDefaultObjectFromObject(UObject* Object)
{
	return IsValid(Object) ? GetMutableDefault<UObject>(Object->GetClass()) : nullptr;
}

UWorld* UCoreBlueprintLibrary::TryGetWorldFromObject(UObject* WorldContextObject)
{
	return GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
}

UWorld* UCoreBlueprintLibrary::TryGetWorldFromObject_K2(UObject* WorldContextObject)
{
	return TryGetWorldFromObject(WorldContextObject);
}
