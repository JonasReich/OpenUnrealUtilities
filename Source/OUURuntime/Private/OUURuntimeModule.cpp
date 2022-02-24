// Copyright (c) 2022 Jonas Reich

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

#if WITH_GAMEPLAY_DEBUGGER
	#include "GameplayAbilities/Debug/GameplayDebuggerCategory_OUUAbilities.h"
	#include "GameplayDebugger/GameplayDebuggerCategoryTypeList.h"
	#include "SequentialFrameScheduler/Debug/GameplayDebuggerCategory_SequentialFrameScheduler.h"

using OUU_GameplayDebuggerCategories = TGameplayDebuggerCategoryTypeList<
	FGameplayDebuggerCategory_OUUAbilities,
	FGameplayDebuggerCategory_SequentialFrameScheduler>;
#endif

class FOUURuntimeModule : public IModuleInterface
{
#if WITH_GAMEPLAY_DEBUGGER
	// - IModuleInterface
	virtual void StartupModule() override
	{
		OUU_GameplayDebuggerCategories::RegisterCategories<EGameplayDebuggerCategoryState::Disabled>();
	}
	virtual void ShutdownModule() override { OUU_GameplayDebuggerCategories::UnregisterCategories(); }
	// --
#endif
};

IMPLEMENT_MODULE(FOUURuntimeModule, OUURuntime)
