// Copyright (c) 2021 Jonas Reich

#include "Core/OUUCoreBlueprintLibrary.h"

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
