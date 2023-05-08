// Copyright (c) 2023 Jonas Reich & Contributors

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
