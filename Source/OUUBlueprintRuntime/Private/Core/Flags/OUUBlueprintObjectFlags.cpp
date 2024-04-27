// Copyright (c) 2023 Jonas Reich & Contributors

#include "Core/Flags/OUUBlueprintObjectFlags.h"

#include "Core/Flags/OUUValidateInlineFiles.h"
#include "Traits/AssertValueEquality.h"

namespace OUU::BlueprintRuntime::Private::BlueprintObjectFlags
{
#define OUU_DECLARE_OBJECT_FLAGS(EnumCase) EOUUBlueprintObjectFlags::EnumCase
	STATIC_ASSERT_INLINE_FLAGS_START(EOUUBlueprintObjectFlags)
#include "Core/Flags/OUUObjectFlagsEnum.inl"
	STATIC_ASSERT_INLINE_FLAGS_END(EOUUBlueprintObjectFlags)
#undef OUU_DECLARE_OBJECT_FLAGS
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
		if (const auto* FlagPtr = BlueprintToNativeFlags.Find(Flag))
			return *FlagPtr;
		return RF_NoFlags;
	}

	EOUUBlueprintObjectFlags ToBlueprintFlag(EObjectFlags Flag)
	{
		if (const auto* FlagPtr = NativeToBlueprintFlags.Find(Flag))
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
} // namespace OUU::BlueprintRuntime::Private::BlueprintObjectFlags

int64 UOUUBlueprintObjectFlagsLibrary::CreateObjectFlagsMask(const TSet<EOUUBlueprintObjectFlags>& Flags)
{
	return static_cast<int64>(OUU::BlueprintRuntime::Private::BlueprintObjectFlags::ToNativeFlags(Flags));
}

TSet<EOUUBlueprintObjectFlags> UOUUBlueprintObjectFlagsLibrary::BreakObjectFlagsMask(int64 Flags)
{
	return OUU::BlueprintRuntime::Private::BlueprintObjectFlags::ToBlueprintFlagsSet(static_cast<EObjectFlags>(Flags));
}

TSet<EOUUBlueprintObjectFlags> UOUUBlueprintObjectFlagsLibrary::GetObjectFlagsSet(const UObject* Object)
{
	return IsValid(Object)
		? OUU::BlueprintRuntime::Private::BlueprintObjectFlags::ToBlueprintFlagsSet(Object->GetFlags())
		: TSet<EOUUBlueprintObjectFlags>{};
}

int64 UOUUBlueprintObjectFlagsLibrary::GetObjectFlagsMask(const UObject* Object)
{
	return IsValid(Object) ? static_cast<int64>(Object->GetFlags()) : 0;
}

bool UOUUBlueprintObjectFlagsLibrary::ObjectHasAnyFlags(const UObject* Object,
	const TSet<EOUUBlueprintObjectFlags>& Flags)
{
	return IsValid(Object)
		? Object->HasAnyFlags(OUU::BlueprintRuntime::Private::BlueprintObjectFlags::ToNativeFlags(Flags))
		: false;
}

bool UOUUBlueprintObjectFlagsLibrary::ObjectHasAllFlags(const UObject* Object,
	const TSet<EOUUBlueprintObjectFlags>& Flags)
{
	return IsValid(Object)
		? Object->HasAllFlags(OUU::BlueprintRuntime::Private::BlueprintObjectFlags::ToNativeFlags(Flags))
		: false;
}
