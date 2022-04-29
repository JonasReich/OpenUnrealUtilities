// Copyright (c) 2022 Jonas Reich

#include "CoreMinimal.h"

#include "Modules/ModuleManager.h"
#include "OUUMapsToCookSettings.h"

class FOUUDeveloperModule : public IModuleInterface
{
	virtual void StartupModule() override { UOUUMapsToCookSettings::TryInjectMapIniSectionCommandlineForCook(); }
};

IMPLEMENT_MODULE(FOUUDeveloperModule, OUUDeveloper)
