// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "GameplayTagContainer.h"

struct OUURUNTIME_API FGameplayTagQueryParser
{
public:
	static FGameplayTagQuery ParseQuery(const FString& SourceString);
};
