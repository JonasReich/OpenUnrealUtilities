// Copyright (c) 2022 Jonas Reich

#include "Localization/ScopedCultureOverride.h"

FScopedCultureOverride::FScopedCultureOverride(const FString& OverrideCultureCode)
{
	auto& Internationalization = FInternationalization::Get();
	Internationalization.BackupCultureState(OriginalCultureState);
	Internationalization.SetCurrentCulture(OverrideCultureCode);
}

FScopedCultureOverride::~FScopedCultureOverride()
{
	auto& Internationalization = FInternationalization::Get();
	Internationalization.RestoreCultureState(OriginalCultureState);
}
