// Copyright (c) 2021 Jonas Reich

#include "OUUEditorModule.h"

#define LOCTEXT_NAMESPACE "FOUUEditorModule"

DEFINE_LOG_CATEGORY(LogOpenUnrealUtilitiesEditor);

void FOUUEditorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FOUUEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FOUUEditorModule, OUUEditor)