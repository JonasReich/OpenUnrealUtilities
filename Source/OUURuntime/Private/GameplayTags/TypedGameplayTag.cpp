// Copyright (c) 2022 Jonas Reich

#include "GameplayTags/TypedGameplayTag.h"

#if WITH_EDITOR
#include "Modules/ModuleManager.h"
	#include "GameplayTagsEditorModule.h"
#include "PropertyEditorModule.h"

namespace OUU::Runtime::Private
{
	void FTypedGameplayTag_Base::RegisterPropertTypeLayout(const FString& TypeName)
	{
		FPropertyEditorModule& PropertyModule =
			FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
		PropertyModule.RegisterCustomPropertyTypeLayout(
			*TypeName.Mid(1),
			FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FGameplayTagCustomizationPublic::MakeInstance));
	}
	void FTypedGameplayTag_Base::UnregisterPropertTypeLayout(const FString& TypeName)
	{
		if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
		{
			FPropertyEditorModule& PropertyModule =
				FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
			PropertyModule.UnregisterCustomPropertyTypeLayout(*TypeName.Mid(1));
		}
	}
} // namespace OUU::Runtime::Private

#endif
