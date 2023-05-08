// Copyright (c) 2023 Jonas Reich & Contributors

#include "CoreMinimal.h"

#include "Animation/Debug/GameplayDebugger_Animation.h"
#include "JsonDataAsset/JsonDataAssetSubsystem.h"
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
	// - IModuleInterface
	virtual void StartupModule() override
	{
		FCoreDelegates::OnAllModuleLoadingPhasesComplete.AddLambda(
			[]() { UJsonDataAssetSubsystem::Get().AddPluginDataRoot(TEXT("OpenUnrealUtilities")); });

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
	virtual void ShutdownModule() override
	{
#if WITH_GAMEPLAY_DEBUGGER
		OUU_GameplayDebuggerCategories::UnregisterCategories();
#endif
	}
	// --
};

IMPLEMENT_MODULE(FOUURuntimeModule, OUURuntime)
