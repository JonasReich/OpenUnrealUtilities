// Copyright (c) 2022 Jonas Reich

#include "Core/Flags/OUUBlueprintObjectFlags.h"

namespace OUUBlueprintObjectFlags_Private
{
	//------------
#define OUU_DECLARE_OBJECT_FLAGS(EnumCase)                                                                             \
	{                                                                                                                  \
		RF_##EnumCase, EOUUBlueprintObjectFlags::EnumCase                                                              \
	}

	TMap<EObjectFlags, EOUUBlueprintObjectFlags> NativeToBlueprintFlags{
#include "Core/Flags/OUUObjectFlagsEnum.inl"
	};
#undef OUU_DECLARE_OBJECT_FLAGS
	//------------
#define OUU_DECLARE_OBJECT_FLAGS(EnumCase)                                                                             \
	{                                                                                                                  \
		EOUUBlueprintObjectFlags::EnumCase, RF_##EnumCase                                                              \
	}
	TMap<EOUUBlueprintObjectFlags, EObjectFlags> BlueprintToNativeFlags{
#include "Core/Flags/OUUObjectFlagsEnum.inl"
	};
#undef OUU_DECLARE_OBJECT_FLAGS
	//------------
#define OUU_DECLARE_OBJECT_FLAGS(EnumCase) RF_##EnumCase
	TArray<EObjectFlags> AllNativeFlags = {
#include "Core/Flags/OUUObjectFlagsEnum.inl"
	};
#undef OUU_DECLARE_OBJECT_FLAGS
	//------------

	EObjectFlags ConvertToNativeFlag(EOUUBlueprintObjectFlags Flag)
	{
		if (auto* FlagPtr = BlueprintToNativeFlags.Find(Flag))
			return *FlagPtr;
		return RF_NoFlags;
	}

	EOUUBlueprintObjectFlags ToBlueprintFlag(EObjectFlags Flag)
	{
		if (auto* FlagPtr = NativeToBlueprintFlags.Find(Flag))
			return *FlagPtr;
		return EOUUBlueprintObjectFlags::NoFlags;
	}

	EObjectFlags ToNativeFlags(TSet<EOUUBlueprintObjectFlags> FlagsSet)
	{
		EObjectFlags ResultFlags = RF_NoFlags;
		for (const EOUUBlueprintObjectFlags Flag : FlagsSet)
		{
			ResultFlags |= ConvertToNativeFlag(Flag);
		}
		return ResultFlags;
	}

	TSet<EOUUBlueprintObjectFlags> ToBlueprintFlagsSet(EObjectFlags InFlags)
	{
		TSet<EOUUBlueprintObjectFlags> ResultSet;
		for (const EObjectFlags Flag : AllNativeFlags)
		{
			if (InFlags & Flag)
			{
				ResultSet.Add(ToBlueprintFlag(Flag));
			}
		}
		return ResultSet;
	}
} // namespace OUUBlueprintObjectFlags_Private

int64 UOUUBlueprintObjectFlagsLibrary::CreateObjectFlagsMask(TSet<EOUUBlueprintObjectFlags> Flags)
{
	return static_cast<int64>(OUUBlueprintObjectFlags_Private::ToNativeFlags(Flags));
}

TSet<EOUUBlueprintObjectFlags> UOUUBlueprintObjectFlagsLibrary::BreakObjectFlagsMask(int64 Flags)
{
	return OUUBlueprintObjectFlags_Private::ToBlueprintFlagsSet(static_cast<EObjectFlags>(Flags));
}

TSet<EOUUBlueprintObjectFlags> UOUUBlueprintObjectFlagsLibrary::GetObjectFlagsSet(const UObject* Object)
{
	return IsValid(Object) ? OUUBlueprintObjectFlags_Private::ToBlueprintFlagsSet(Object->GetFlags())
						   : TSet<EOUUBlueprintObjectFlags>{};
}

int64 UOUUBlueprintObjectFlagsLibrary::GetObjectFlagsMask(const UObject* Object)
{
	return IsValid(Object) ? static_cast<int64>(Object->GetFlags()) : 0;
}

bool UOUUBlueprintObjectFlagsLibrary::ObjectHasAnyFlags(const UObject* Object, TSet<EOUUBlueprintObjectFlags> Flags)
{
	return IsValid(Object) ? Object->HasAnyFlags(OUUBlueprintObjectFlags_Private::ToNativeFlags(Flags)) : false;
}

bool UOUUBlueprintObjectFlagsLibrary::ObjectHasAllFlags(const UObject* Object, TSet<EOUUBlueprintObjectFlags> Flags)
{
	return IsValid(Object) ? Object->HasAllFlags(OUUBlueprintObjectFlags_Private::ToNativeFlags(Flags)) : false;
}
