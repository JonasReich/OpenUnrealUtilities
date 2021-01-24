// Copyright (c) 2021 Jonas Reich

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

OUUTESTS_API DECLARE_LOG_CATEGORY_EXTERN(LogOpenUnrealUtilitiesTests, Log, All);

class FOUUTestsModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
