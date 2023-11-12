// Copyright (c) 2023 Jonas Reich & Contributors

#include "CoreMinimal.h"

#include "ActorMapWindow/OUUActorMapWindow.h"
#include "Modules/ModuleManager.h"

class FOUUDeveloperModule : public IModuleInterface
{
	void StartupModule() override { OUU::Developer::ActorMapWindow::RegisterNomadTabSpawner(); }
	void ShutdownModule() override { OUU::Developer::ActorMapWindow::UnregisterNomadTabSpawner(); }
};

IMPLEMENT_MODULE(FOUUDeveloperModule, OUUDeveloper)
