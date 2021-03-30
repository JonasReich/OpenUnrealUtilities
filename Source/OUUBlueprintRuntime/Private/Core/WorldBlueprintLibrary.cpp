// Copyright (c) 2021 Jonas Reich

#include "Core/WorldBlueprintLibrary.h"

EBlueprintWorldType UWorldBlueprintLibrary::GetWorldType(UWorld* World)
{
	if (!IsValid(World))
		return EBlueprintWorldType::None;

	return StaticCast<EBlueprintWorldType>(StaticCast<uint8>(World->WorldType));
}
