// Copyright (c) 2021 Jonas Reich

#pragma once

#include "GameplayTagContainer.h"

struct OUURUNTIME_API FGameplayTagQueryParser
{
public:
	static FGameplayTagQuery ParseQuery(const FString& SourceString);
};
