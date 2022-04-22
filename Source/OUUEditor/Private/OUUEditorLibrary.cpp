// Copyright (c) 2022 Jonas Reich

#include "OUUEditorLibrary.h"

#include "ISessionFrontendModule.h"
#include "Modules/ModuleManager.h"

void UOUUEditorLibrary::InvokeSessionFrontend(FName Panel)
{
	ISessionFrontendModule& SessionFrontend =
		FModuleManager::LoadModuleChecked<ISessionFrontendModule>("SessionFrontend");
	SessionFrontend.InvokeSessionFrontend(Panel);
}
