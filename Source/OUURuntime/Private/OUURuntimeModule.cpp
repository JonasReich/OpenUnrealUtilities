// Copyright (c) 2022 Jonas Reich

#include "CoreMinimal.h"

#include "Animation/Debug/GameplayDebugger_Animation.h"
#include "Modules/ModuleManager.h"

#if WITH_GAMEPLAY_DEBUGGER
	#include "GameplayAbilities/Debug/GameplayDebuggerCategory_OUUAbilities.h"
	#include "GameplayDebugger/GameplayDebuggerCategoryTypeList.h"
	#include "GameplayDebugger/GameplayDebuggerCategory_ViewModes.h"
	#include "GameplayDebugger/GameplayDebuggerExtension_ActorSelect.h"
	#include "SequentialFrameScheduler/Debug/GameplayDebuggerCategory_SequentialFrameScheduler.h"

using OUU_GameplayDebuggerCategories = TGameplayDebuggerCategoryTypeList<
	FGameplayDebuggerCategory_OUUAbilities,
	FGameplayDebuggerCategory_SequentialFrameScheduler,
	FGameplayDebuggerCategory_Animation,
	FGameplayDebuggerCategory_ViewModes>;
#endif

class FOUURuntimeModule : public IModuleInterface
{
#if WITH_GAMEPLAY_DEBUGGER
	// - IModuleInterface
	virtual void StartupModule() override
	{
		OUU_GameplayDebuggerCategories::RegisterCategories<EGameplayDebuggerCategoryState::Disabled>();

		// #TODO-OUU Extract TGameplayDebuggerCategoryTypeList to template that is also usable for other extensions,
		// so we can get rid of this boilerplate as well
		IGameplayDebugger::Get()
			.RegisterExtension("ActorSelect", IGameplayDebugger::FOnGetExtension::CreateLambda([]() {
								   return MakeShared<FGameplayDebuggerExtension_ActorSelect>();
							   }));
	}
	virtual void ShutdownModule() override { OUU_GameplayDebuggerCategories::UnregisterCategories(); }
	// --
#endif
};

IMPLEMENT_MODULE(FOUURuntimeModule, OUURuntime)
