// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "Templates/BitmaskUtils.h"

/**
 * The following two macros can be used to validate the .inl files for blueprint flags.
 * They assert that the number of flags declarations in the public header file matches to the number of entries in the
 * .inl file.
 * We also cannot enforce that every value from native UE source has a matching Blueprint equivalent,
 * because there are no meta flags for the count of flags in UE source.
 */

// put this before the inline include
#define STATIC_ASSERT_INLINE_FLAGS_START(FlagsType)                                                                    \
	constexpr std::initializer_list<FlagsType> FlagsList                                                               \
	{
// put this after the inline inlucde
#define STATIC_ASSERT_INLINE_FLAGS_END(FlagsType)                                                                      \
	}                                                                                                                  \
	;                                                                                                                  \
	static_assert(                                                                                                     \
		TAssertValuesEqual<int32, FlagsList.size(), static_cast<int32>(FlagsType::META_NumFlags)>::Value,              \
		"Mismatch of count of flags declarations in " PREPROCESSOR_TO_STRING(                                          \
			FlagsType) " and entries in inline file");                                                                 \
	static_assert(                                                                                                     \
		TOr<TAssertValuesEqual<                                                                                        \
				int32,                                                                                                 \
				FlagsList.size(),                                                                                      \
				OUU::BlueprintRuntime::Private::Flags::NumUniqueValues<FlagsType>(FlagsList)>,                         \
			OUU::BlueprintRuntime::Private::Flags::TAssertErrorShowEnumValue<                                          \
				FlagsType,                                                                                             \
				OUU::BlueprintRuntime::Private::Flags::GetFirstNonUniqueValue<FlagsType>(FlagsList)>>::Value,          \
		"Initializer list from inline file contains duplicate values. "                                                \
		"Check the last enum value referenced in error message above to find out which enum case is affected by "      \
		"this.");

namespace OUU::BlueprintRuntime::Private::Flags
{
	template <typename T>
	constexpr int32 NumUniqueValues(const std::initializer_list<T>& FlagsList)
	{
		// ReSharper disable once CppTooWideScope
		uint64 CheckBitmask = 0;
		int32 NumUniqueElements = 0;
		for (int32 i = 0; i < FlagsList.size(); ++i)
		{
			auto Entry = *(FlagsList.begin() + i);
			const uint64 BitmaskBefore = CheckBitmask;
			OUU::Runtime::BitmaskUtils::SetBit<T, EEnumSequenceType::Natural, uint64>(CheckBitmask, Entry);
			if (CheckBitmask != BitmaskBefore)
			{
				NumUniqueElements++;
			}
		}
		return NumUniqueElements;
	}

	// Returns default value when there are only unique values
	template <typename T>
	constexpr T GetFirstNonUniqueValue(const std::initializer_list<T>& FlagsList)
	{
		// ReSharper disable once CppTooWideScope
		uint64 CheckBitmask = 0;
		for (int32 i = 0; i < FlagsList.size(); ++i)
		{
			auto Entry = *(FlagsList.begin() + i);
			const uint64 BitmaskBefore = CheckBitmask;
			OUU::Runtime::BitmaskUtils::SetBit<T, EEnumSequenceType::Natural, uint64>(CheckBitmask, Entry);
			if (CheckBitmask == BitmaskBefore)
			{
				return Entry;
			}
		}
		return T{};
	}

	template <typename T, T A>
	struct TAssertErrorShowEnumValue
	{
		static const bool Value = static_cast<int32>(A) == -1;
	};
} // namespace OUU::BlueprintRuntime::Private::Flags
