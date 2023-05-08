// Copyright (c) 2023 Jonas Reich & Contributors

#include "Core/Flags/OUUBlueprintClassFlags.h"

#include "OUUValidateInlineFiles.h"

namespace OUU::BlueprintRuntime::Private::BlueprintClassFlags
{
#define OUU_DECLARE_CLASS_FLAGS(EnumCase) EOUUBlueprintClassFlags::EnumCase
	STATIC_ASSERT_INLINE_FLAGS_START(EOUUBlueprintClassFlags)
#include "Core/Flags/OUUClassFlagsEnum.inl"
	STATIC_ASSERT_INLINE_FLAGS_END(EOUUBlueprintClassFlags)
#undef OUU_DECLARE_CLASS_FLAGS
	//------------
#define OUU_DECLARE_CLASS_FLAGS(EnumCase)                                                                              \
	{                                                                                                                  \
		CLASS_##EnumCase, EOUUBlueprintClassFlags::EnumCase                                                            \
	}

	TMap<EClassFlags, EOUUBlueprintClassFlags> NativeToBlueprintFlags{
#include "Core/Flags/OUUClassFlagsEnum.inl"
	};
#undef OUU_DECLARE_CLASS_FLAGS
	//------------
#define OUU_DECLARE_CLASS_FLAGS(EnumCase)                                                                              \
	{                                                                                                                  \
		EOUUBlueprintClassFlags::EnumCase, CLASS_##EnumCase                                                            \
	}
	TMap<EOUUBlueprintClassFlags, EClassFlags> BlueprintToNativeFlags{
#include "Core/Flags/OUUClassFlagsEnum.inl"
	};
#undef OUU_DECLARE_CLASS_FLAGS
	//------------
#define OUU_DECLARE_CLASS_FLAGS(EnumCase) CLASS_##EnumCase
	TArray<EClassFlags> AllNativeFlags = {
#include "Core/Flags/OUUClassFlagsEnum.inl"
	};
#undef OUU_DECLARE_CLASS_FLAGS
	//------------

	EClassFlags ConvertToNativeFlag(EOUUBlueprintClassFlags Flag)
	{
		if (auto* FlagPtr = BlueprintToNativeFlags.Find(Flag))
			return *FlagPtr;
		return EClassFlags::CLASS_None;
	}

	EOUUBlueprintClassFlags ToBlueprintFlag(EClassFlags Flag)
	{
		if (auto* FlagPtr = NativeToBlueprintFlags.Find(Flag))
			return *FlagPtr;
		return EOUUBlueprintClassFlags::None;
	}

	EClassFlags ToNativeFlags(TSet<EOUUBlueprintClassFlags> FlagsSet)
	{
		EClassFlags ResultFlags = EClassFlags::CLASS_None;
		for (const EOUUBlueprintClassFlags Flag : FlagsSet)
		{
			ResultFlags |= ConvertToNativeFlag(Flag);
		}
		return ResultFlags;
	}

	TSet<EOUUBlueprintClassFlags> ToBlueprintFlagsSet(EClassFlags InFlags)
	{
		TSet<EOUUBlueprintClassFlags> ResultSet;
		for (const EClassFlags Flag : AllNativeFlags)
		{
			if (InFlags & Flag)
			{
				ResultSet.Add(ToBlueprintFlag(Flag));
			}
		}
		return ResultSet;
	}
} // namespace OUU::BlueprintRuntime::Private::BlueprintClassFlags

int64 UOUUBlueprintClassFlagsLibrary::CreateClassFlagsMask(TSet<EOUUBlueprintClassFlags> Flags)
{
	return static_cast<int64>(OUU::BlueprintRuntime::Private::BlueprintClassFlags::ToNativeFlags(Flags));
}

TSet<EOUUBlueprintClassFlags> UOUUBlueprintClassFlagsLibrary::BreakClassFlagsMask(int64 Flags)
{
	return OUU::BlueprintRuntime::Private::BlueprintClassFlags::ToBlueprintFlagsSet(static_cast<EClassFlags>(Flags));
}

TSet<EOUUBlueprintClassFlags> UOUUBlueprintClassFlagsLibrary::GetClassFlagsSet(const UClass* Class)
{
	return IsValid(Class)
		? OUU::BlueprintRuntime::Private::BlueprintClassFlags::ToBlueprintFlagsSet(Class->GetClassFlags())
		: TSet<EOUUBlueprintClassFlags>{};
}

int64 UOUUBlueprintClassFlagsLibrary::GetClassFlagsMask(const UClass* Class)
{
	return IsValid(Class) ? Class->GetClassFlags() : 0;
}

bool UOUUBlueprintClassFlagsLibrary::ClassHasAnyFlags(const UClass* Class, TSet<EOUUBlueprintClassFlags> Flags)
{
	return IsValid(Class)
		? Class->HasAnyClassFlags(OUU::BlueprintRuntime::Private::BlueprintClassFlags::ToNativeFlags(Flags))
		: false;
}

bool UOUUBlueprintClassFlagsLibrary::ClassHasAllFlags(const UClass* Class, TSet<EOUUBlueprintClassFlags> Flags)
{
	return IsValid(Class)
		? Class->HasAllClassFlags(OUU::BlueprintRuntime::Private::BlueprintClassFlags::ToNativeFlags(Flags))
		: false;
}
