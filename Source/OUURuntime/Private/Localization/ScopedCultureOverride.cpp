// Copyright (c) 2023 Jonas Reich & Contributors

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
