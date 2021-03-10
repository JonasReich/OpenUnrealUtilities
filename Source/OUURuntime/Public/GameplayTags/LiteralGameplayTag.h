// Copyright (c) 2021 Jonas Reich

#pragma once

#include "GameplayTagContainer.h"
#include "GameplayTagsManager.h"
#include "Templates/UnrealTypeTraits.h"

/**
 * --------------------------
 *  Literal Gameplay Tags
 * --------------------------
 * 
 * Literal tags offer the possibility to map gameplay tags defined in DefaultGameplayTags.ini to
 * compile time types that ensure compile time safety.
 * There is no guarantee that matching gameplay tags exist in the ini files, but you can at least
 * ensure that you don't end up with typos in the tags without having to come up with new names.
 *
 * Usage:
 * You need to declare gameplay tag collections like so:
 *
 *     struct FSampleGameplayTags : public FLiteralGameplayTagRoot
 *     {
 *         OUU_GTAG(Player);
 *         OUU_GTAG_GROUP(NPC)
 *             OUU_GTAG_GROUP(State)
 *                 OUU_GTAG(Idle);
 *                 OUU_GTAG(Combat);
 *                 OUU_GTAG(Moving);
 *             };
 *         };
 *     };
 *
 * The actual tags can then be obtained like this:
 *
 *     FGameplayTag IdleTag = FSampleGameplayTags::NPC::State::Idle::Get();
 *     FGameplayTag NPCTag = FSampleGameplayTags::NPC::Get();
 *
 * As you can see, these code tags use the C++ scope-resolution operator '::' instead of the dot '.',
 * which has the advantage that you don't need any instances when getting tags.
 */

/** Base class for the root of literal gameplay tag type hierarchies. */
OUURUNTIME_API struct FLiteralGameplayTagRoot
{
	// These members of this struct are just required so derived types can act as an
	// outer/parent tag type to the derived types of TLiteralGameplayTag.
protected:
	template <typename, typename>
	friend struct TLiteralGameplayTag;

	using SelfTagType = FLiteralGameplayTagRoot;
	static FString GetName() { return FString(""); }
};

struct CHasRelativeTagName
{
	template <typename T>
	auto Requires() -> decltype(
		FString(T::GetRelativeName())
	);
};

struct CIsLiteralGameplayTag
{
	template <typename A, typename B>
	void CheckBase(TLiteralGameplayTag<A, B>&&);

	template <typename T>
	auto Requires() -> decltype(
		CheckBase(DeclVal<T>())
	);
};

/**
 * The base class for literal gameplay tag types.
 * Derived types should be declared with the GTAG and GTAG_GROUP macros defined below.1
 */
template <typename InSelfTagType, typename InParentTagType>
struct TLiteralGameplayTag
{
	using ThisType = TLiteralGameplayTag<InSelfTagType, InParentTagType>;
	using SelfTagType = InSelfTagType;
	using ParentTagType = InParentTagType;

	static_assert(TIsDerivedFrom<ParentTagType, FLiteralGameplayTagRoot>::Value || TModels<CIsLiteralGameplayTag, ParentTagType>::Value,
		"ParentTagType must be either derived from FLiteralGameplayTagRoot or from LiteralGameplayTag");

	static FString GetName()
	{
		static_assert(TModels<CHasRelativeTagName, InSelfTagType>::Value, "SelfTagType must have member GetRelativeName().\n"
			"It's strongly recommended to only create these derived types via the GTAG and GTAG_GROUP macros.");

		static_assert(TIsDerivedFrom<SelfTagType, ThisType>::Value && !TIsSame<ThisType, SelfTagType>::Value,
			"InSelfTagType must be a struct type derived from TLiteralGameplayTag, but not TLiteralGameplayTag itself.\n"
			"It's strongly recommended to only create these derived types via the GTAG and GTAG_GROUP macros.");

		return (ParentTagType::GetName().Len() > 0) ? (ParentTagType::GetName() + FString(".") + SelfTagType::GetRelativeName()) : SelfTagType::GetRelativeName();
	}

	static FGameplayTag Get()
	{
		static FGameplayTag Tag = UGameplayTagsManager::Get().RequestGameplayTag(*GetName());
		return Tag;
	}
};

#define OUU_LITERAL_GTAG_START_IMPL(Tag, TagNameString, ParentTag) \
struct Tag : public TLiteralGameplayTag<Tag, ParentTag>\
{ \
	static FString GetRelativeName() { return FString(TagNameString); }

/**
 * Create a tag that contains other tags. You have to close the group with };
 * Example:
 *    OUU_GTAG_GROUP(Foo)
 *        OUU_GTAG(Bar);
 *        OUU_GTAG(FooBar);
 *    };
 */
#define OUU_GTAG_GROUP(TagName) \
	OUU_LITERAL_GTAG_START_IMPL(TagName, PREPROCESSOR_TO_STRING(TagName), SelfTagType)

/**
 * Create a leaf tag that does not contain other tags in the literal tags hierarchy in C++.
 * The "real" tag equivalent configured via GameplayTags.ini may contain sub-tags,
 * but those sub-tags won't be available via literal gameplay tags then.
 */
#define OUU_GTAG(TagName) \
	OUU_LITERAL_GTAG_START_IMPL(TagName, PREPROCESSOR_TO_STRING(TagName), SelfTagType) };
