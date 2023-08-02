// Copyright (c) 2023 Jonas Reich & Contributors

#include "GameplayTags/LiteralGameplayTag.h"

namespace OUU::Editor::Private
{
#if WITH_EDITOR
	TMap<FGameplayTag, ELiteralGameplayTagFlags> GLiteralTagFlags;
	struct FTagSource
	{
		FName PluginName;
		FName ModuleName;

		FString ToString() const
		{
			return FString::Printf(TEXT("%s.%s"), *PluginName.ToString(), *ModuleName.ToString());
		}
	};
	TMap<FGameplayTag, FTagSource> GTagSources;

	void RegisterLiteralTagFlagsForValidation(
		FGameplayTag Tag,
		ELiteralGameplayTagFlags Flags,
		FName PluginName,
		FName ModuleName)
	{
		FTagSource Source{PluginName, ModuleName};

		checkf(
			GLiteralTagFlags.Contains(Tag) == false,
			TEXT("%s is registered both by %s and %s. Please only register it from one declartion!"),
			*Tag.ToString(),
			*Source.ToString(),
			*GTagSources[Tag].ToString());

		GLiteralTagFlags.Add(Tag, Flags);
		GTagSources.Add(Tag, Source);
	}
	const TMap<FGameplayTag, ELiteralGameplayTagFlags>& GetTagFlagsForValidation() { return GLiteralTagFlags; }
#endif
} // namespace OUU::Editor::Private
