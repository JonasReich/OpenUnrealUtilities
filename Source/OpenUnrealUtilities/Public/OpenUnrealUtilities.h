// Copyright (c) 2020 Jonas Reich

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FOpenUnrealUtilitiesModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
