// Copyright (c) 2023 Jonas Reich & Contributors

#include "Core/Flags/OUUBlueprintPropertyFlags.h"

#include "OUUValidateInlineFiles.h"
#include "Traits/AssertValueEquality.h"

namespace OUU::BlueprintRuntime::Private::BlueprintPropertyFlags
{
#define OUU_DECLARE_PROPERTY_FLAGS(EnumCase) EOUUBlueprintPropertyFlags::EnumCase
	STATIC_ASSERT_INLINE_FLAGS_START(EOUUBlueprintPropertyFlags)
#include "Core/Flags/OUUPropertyFlagsEnum.inl"
	STATIC_ASSERT_INLINE_FLAGS_END(EOUUBlueprintPropertyFlags)
#undef OUU_DECLARE_PROPERTY_FLAGS
	//------------
#define OUU_DECLARE_PROPERTY_FLAGS(EnumCase)                                                                           \
	{                                                                                                                  \
		CPF_##EnumCase, EOUUBlueprintPropertyFlags::EnumCase                                                           \
	}

	const TMap<EPropertyFlags, EOUUBlueprintPropertyFlags> NativeToBlueprintFlags{
#include "Core/Flags/OUUPropertyFlagsEnum.inl"
	};
#undef OUU_DECLARE_PROPERTY_FLAGS
	//------------
#define OUU_DECLARE_PROPERTY_FLAGS(EnumCase)                                                                           \
	{                                                                                                                  \
		EOUUBlueprintPropertyFlags::EnumCase, CPF_##EnumCase                                                           \
	}
	const TMap<EOUUBlueprintPropertyFlags, EPropertyFlags> BlueprintToNativeFlags{
#include "Core/Flags/OUUPropertyFlagsEnum.inl"
	};
#undef OUU_DECLARE_PROPERTY_FLAGS
	//------------
#define OUU_DECLARE_PROPERTY_FLAGS(EnumCase) CPF_##EnumCase
	const TArray<EPropertyFlags> AllNativeFlags = {
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
} // namespace OUU::BlueprintRuntime::Private::BlueprintPropertyFlags

int64 UOUUBlueprintPropertyFlagsLibrary::CreatePropertyFlagsMask(TSet<EOUUBlueprintPropertyFlags> Flags)
{
	return static_cast<int64>(OUU::BlueprintRuntime::Private::BlueprintPropertyFlags::ToNativeFlags(Flags));
}

TSet<EOUUBlueprintPropertyFlags> UOUUBlueprintPropertyFlagsLibrary::BreakPropertyFlagsMask(int64 Flags)
{
	return OUU::BlueprintRuntime::Private::BlueprintPropertyFlags::ToBlueprintFlagsSet(
		static_cast<EPropertyFlags>(Flags));
}
