// Copyright (c) 2023 Jonas Reich & Contributors

#include "GameplayTags/LiteralGameplayTag.h"

namespace OUU::Editor::Private
{
#if WITH_EDITOR
	TMap<FGameplayTag, ELiteralGameplayTagFlags> GLiteralTagFlags;

	void RegisterLiteralTagFlagsForValidation(FGameplayTag Tag, ELiteralGameplayTagFlags Flags)
	{
		GLiteralTagFlags.Add(Tag, Flags);
	}
	const TMap<FGameplayTag, ELiteralGameplayTagFlags>& GetTagFlagsForValidation() { return GLiteralTagFlags; }
#endif
} // namespace OUU::Editor::Private