// Copyright (c) 2022 Jonas Reich

#pragma once

#include "CoreMinimal.h"

#include "Internationalization/Internationalization.h"

struct OUURUNTIME_API FScopedCultureOverride
{
public:
	FScopedCultureOverride(const FString& OverrideCultureCode);
	~FScopedCultureOverride();

private:
	FInternationalization::FCultureStateSnapshot OriginalCultureState;
};
