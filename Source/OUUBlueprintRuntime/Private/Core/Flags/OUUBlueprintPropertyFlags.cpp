// Copyright (c) 2022 Jonas Reich

#include "Core/Flags/OUUBlueprintPropertyFlags.h"

namespace OUUBlueprintPropertyFlags_Private
{
	//------------
#define OUU_DECLARE_PROPERTY_FLAGS(EnumCase)                                                                           \
	{                                                                                                                  \
		CPF_##EnumCase, EOUUBlueprintPropertyFlags::EnumCase                                                           \
	}

	TMap<EPropertyFlags, EOUUBlueprintPropertyFlags> NativeToBlueprintFlags{
#include "Core/Flags/OUUPropertyFlagsEnum.inl"
	};
#undef OUU_DECLARE_PROPERTY_FLAGS
	//------------
#define OUU_DECLARE_PROPERTY_FLAGS(EnumCase)                                                                           \
	{                                                                                                                  \
		EOUUBlueprintPropertyFlags::EnumCase, CPF_##EnumCase                                                           \
	}
	TMap<EOUUBlueprintPropertyFlags, EPropertyFlags> BlueprintToNativeFlags{
#include "Core/Flags/OUUPropertyFlagsEnum.inl"
	};
#undef OUU_DECLARE_PROPERTY_FLAGS
	//------------
#define OUU_DECLARE_PROPERTY_FLAGS(EnumCase) CPF_##EnumCase
	TArray<EPropertyFlags> AllNativeFlags = {
#include "Core/Flags/OUUPropertyFlagsEnum.inl"
	};
#undef OUU_DECLARE_PROPERTY_FLAGS
	//------------

	EPropertyFlags ConvertToNativeFlag(EOUUBlueprintPropertyFlags Flag)
	{
		if (auto* FlagPtr = BlueprintToNativeFlags.Find(Flag))
			return *FlagPtr;
		return EPropertyFlags::CPF_None;
	}

	EOUUBlueprintPropertyFlags ToBlueprintFlag(EPropertyFlags Flag)
	{
		if (auto* FlagPtr = NativeToBlueprintFlags.Find(Flag))
			return *FlagPtr;
		return EOUUBlueprintPropertyFlags::None;
	}

	EPropertyFlags ToNativeFlags(TSet<EOUUBlueprintPropertyFlags> FlagsSet)
	{
		EPropertyFlags ResultFlags = EPropertyFlags::CPF_None;
		for (const EOUUBlueprintPropertyFlags Flag : FlagsSet)
		{
			ResultFlags |= ConvertToNativeFlag(Flag);
		}
		return ResultFlags;
	}

	TSet<EOUUBlueprintPropertyFlags> ToBlueprintFlagsSet(EPropertyFlags InFlags)
	{
		TSet<EOUUBlueprintPropertyFlags> ResultSet;
		for (const EPropertyFlags Flag : AllNativeFlags)
		{
			if (InFlags & Flag)
			{
				ResultSet.Add(ToBlueprintFlag(Flag));
			}
		}
		return ResultSet;
	}
} // namespace OUUBlueprintPropertyFlags_Private

int64 UOUUBlueprintPropertyFlagsLibrary::CreatePropertyFlagsMask(TSet<EOUUBlueprintPropertyFlags> Flags)
{
	return static_cast<int64>(OUUBlueprintPropertyFlags_Private::ToNativeFlags(Flags));
}

TSet<EOUUBlueprintPropertyFlags> UOUUBlueprintPropertyFlagsLibrary::BreakPropertyFlagsMask(int64 Flags)
{
	return OUUBlueprintPropertyFlags_Private::ToBlueprintFlagsSet(static_cast<EPropertyFlags>(Flags));
}
