// Copyright (c) 2025 Jonas Reich & Contributors

#include "Engine/UserInterfaceSettings.h"

namespace
{
	void SetSlateApplicationScale(const TArray<FString>& Args)
	{
		if (Args.Num() > 0)
		{
			float Scale = 1.0f;
			LexFromString(Scale, *Args[0]);
			GetMutableDefault<UUserInterfaceSettings>()->ApplicationScale = Scale;
		}
	}

	FAutoConsoleCommand CCommand_SetSlateApplicationScale{
		TEXT("ouu.Slate.ApplicationScale"),
		TEXT("Override the UI settings application scale to quickly test different resolution scales"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&SetSlateApplicationScale),
		ECVF_Default};
} // namespace
