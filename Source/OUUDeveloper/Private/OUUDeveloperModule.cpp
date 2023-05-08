// Copyright (c) 2023 Jonas Reich & Contributors

#include "CoreMinimal.h"

#include "ActorMapWindow/OUUActorMapWindow.h"
#include "Modules/ModuleManager.h"
#include "OUUMapsToCookSettings.h"

class FOUUDeveloperModule : public IModuleInterface
{
	virtual void StartupModule() override { OUU::Developer::ActorMapWindow::RegisterNomadTabSpawner(); }

	virtual void ShutdownModule() override { OUU::Developer::ActorMapWindow::UnregisterNomadTabSpawner(); }
};

IMPLEMENT_MODULE(FOUUDeveloperModule, OUUDeveloper)
