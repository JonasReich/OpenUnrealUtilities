// Copyright (c) 2022 Jonas Reich

#include "CoreMinimal.h"

#include "ActorMapWindow/OUUActorMapWindow.h"
#include "Modules/ModuleManager.h"
#include "OUUMapsToCookSettings.h"

class FOUUDeveloperModule : public IModuleInterface
{
	virtual void StartupModule() override
	{
		OUU::Developer::ActorMapWindow::RegisterNomadTabSpawner();
		UOUUMapsToCookSettings::TryInjectMapIniSectionCommandlineForCook();
	}

	virtual void ShutdownModule() override { OUU::Developer::ActorMapWindow::UnregisterNomadTabSpawner(); }
};

IMPLEMENT_MODULE(FOUUDeveloperModule, OUUDeveloper)
