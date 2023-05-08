// Copyright (c) 2023 Jonas Reich & Contributors

#include "ApplicationCore/ClipboardLibrary.h"

#include "HAL/PlatformApplicationMisc.h"

void UClipboardLibrary::ClipboardCopy(const FString& SourceString)
{
	FPlatformApplicationMisc::ClipboardCopy(*SourceString);
}

FString UClipboardLibrary::ClipboardPaste()
{
	FString Result;
	FPlatformApplicationMisc::ClipboardPaste(Result);
	return Result;
}
