// Copyright (c) 2021 Jonas Reich

#include "Core/EngineGlobalsLibrary.h"

bool UEngineGlobalsLibrary::IsEditorBuild()
{
	return static_cast<bool>(WITH_EDITOR);
}

bool UEngineGlobalsLibrary::IsDebugBuild()
{
	return static_cast<bool>(UE_BUILD_DEBUG);
}

bool UEngineGlobalsLibrary::IsDevelopmentBuild()
{
	return static_cast<bool>(UE_BUILD_DEVELOPMENT);
}

bool UEngineGlobalsLibrary::IsTestBuild()
{
	return static_cast<bool>(UE_BUILD_TEST);
}

bool UEngineGlobalsLibrary::IsShippingBuild()
{
	return static_cast<bool>(UE_BUILD_SHIPPING);
}

bool UEngineGlobalsLibrary::IsRunningCommandlet()
{
	return ::IsRunningCommandlet();
}

bool UEngineGlobalsLibrary::AllowCommandletRendering()
{
	return ::IsAllowCommandletRendering();
}

bool UEngineGlobalsLibrary::AllowCommandletAudio()
{
	return ::IsAllowCommandletAudio();
}

bool UEngineGlobalsLibrary::IsEditor()
{
	return GIsEditor;
}

bool UEngineGlobalsLibrary::IsClient()
{
	return GIsClient;
}

bool UEngineGlobalsLibrary::IsServer()
{
	return GIsServer;
}

bool UEngineGlobalsLibrary::IsBuildMachine()
{
	return GIsBuildMachine;
}

bool UEngineGlobalsLibrary::IsInGameThread()
{
	return ::IsInGameThread();
}
