// Copyright (c) 2023 Jonas Reich & Contributors

#include "CoreMinimal.h"

#include "Animation/Debug/GameplayDebugger_Animation.h"
#include "Modules/ModuleManager.h"

#if WITH_GAMEPLAY_DEBUGGER
	#include "GameEntitlements/Debug/GameplayDebuggerCategory_GameEntitlements.h"
	#include "GameplayAbilities/Debug/GameplayDebuggerCategory_OUUAbilities.h"
	#include "GameplayDebugger/GameplayDebuggerCategoryTypeList.h"
	#include "GameplayDebugger/GameplayDebuggerCategory_ViewModes.h"
	#include "GameplayDebugger/GameplayDebuggerExtension_ActorSelect.h"
	#include "SequentialFrameScheduler/Debug/GameplayDebuggerCategory_SequentialFrameScheduler.h"

using OUU_GameplayDebuggerCategories = TGameplayDebuggerCategoryTypeList<
	FGameplayDebuggerCategory_OUUAbilities,
	FGameplayDebuggerCategory_SequentialFrameScheduler,
	FGameplayDebuggerCategory_Animation,
	FGameplayDebuggerCategory_GameEntitlements,
	FGameplayDebuggerCategory_ViewModes>;
#endif

class FOUURuntimeModule : public IModuleInterface
{
	// - IModuleInterface
	void StartupModule() override
	{
#if WITH_GAMEPLAY_DEBUGGER
		OUU_GameplayDebuggerCategories::RegisterCategories<EGameplayDebuggerCategoryState::Disabled>();

		// #TODO-OUU Extract TGameplayDebuggerCategoryTypeList to template that is also usable for other extensions,
		// so we can get rid of this boilerplate as well
		IGameplayDebugger::Get()
			.RegisterExtension("ActorSelect", IGameplayDebugger::FOnGetExtension::CreateLambda([]() {
								   return MakeShared<FGameplayDebuggerExtension_ActorSelect>();
							   }));

#endif
	}
	void ShutdownModule() override
	{
#if WITH_GAMEPLAY_DEBUGGER
		OUU_GameplayDebuggerCategories::UnregisterCategories();
#endif
	}
	// --
};

IMPLEMENT_MODULE(FOUURuntimeModule, OUURuntime)
