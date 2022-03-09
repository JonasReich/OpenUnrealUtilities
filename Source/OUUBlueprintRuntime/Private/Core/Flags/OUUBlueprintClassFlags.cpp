// Copyright (c) 2022 Jonas Reich

#include "Core/Flags/OUUBlueprintClassFlags.h"

namespace OUUBlueprintClassFlags_Private
{
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
} // namespace OUUBlueprintClassFlags_Private

int64 UOUUBlueprintClassFlagsLibrary::CreateClassFlagsMask(TSet<EOUUBlueprintClassFlags> Flags)
{
	return static_cast<int64>(OUUBlueprintClassFlags_Private::ToNativeFlags(Flags));
}

TSet<EOUUBlueprintClassFlags> UOUUBlueprintClassFlagsLibrary::BreakClassFlagsMask(int64 Flags)
{
	return OUUBlueprintClassFlags_Private::ToBlueprintFlagsSet(static_cast<EClassFlags>(Flags));
}

TSet<EOUUBlueprintClassFlags> UOUUBlueprintClassFlagsLibrary::GetClassFlagsSet(const UClass* Class)
{
	return IsValid(Class) ? OUUBlueprintClassFlags_Private::ToBlueprintFlagsSet(Class->GetClassFlags())
						  : TSet<EOUUBlueprintClassFlags>{};
}

int64 UOUUBlueprintClassFlagsLibrary::GetClassFlagsMask(const UClass* Class)
{
	return IsValid(Class) ? Class->GetClassFlags() : 0;
}

bool UOUUBlueprintClassFlagsLibrary::ClassHasAnyFlags(const UClass* Class, TSet<EOUUBlueprintClassFlags> Flags)
{
	return IsValid(Class) ? Class->HasAnyClassFlags(OUUBlueprintClassFlags_Private::ToNativeFlags(Flags)) : false;
}

bool UOUUBlueprintClassFlagsLibrary::ClassHasAllFlags(const UClass* Class, TSet<EOUUBlueprintClassFlags> Flags)
{
	return IsValid(Class) ? Class->HasAllClassFlags(OUUBlueprintClassFlags_Private::ToNativeFlags(Flags)) : false;
}
