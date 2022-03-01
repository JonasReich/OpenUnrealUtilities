// Copyright (c) 2022 Jonas Reich

#pragma once

#include "CoreMinimal.h"

#include "GameplayTagContainer.h"
#include "GameplayTagsManager.h"
#include "GameplayTagsModule.h"
#include "Misc/EngineVersionComparison.h"
#include "Templates/UnrealTypeTraits.h"

// If the NativeGameplayTags are available.
// They are not necessarily required for the literal gameplay tags, but without them the tooltip descriptions won't be
// properly updated.
#define WITH_NATIVE_GAMEPLAY_TAGS UE_VERSION_NEWER_THAN(4, 27, 0)

#if WITH_NATIVE_GAMEPLAY_TAGS
	#include "NativeGameplayTags.h"
#endif

/**
 * --------------------------
 *  Literal (Native) Gameplay Tags
 * --------------------------
 *
 * Literal tags offer the possibility to map gameplay tags defined in DefaultGameplayTags.ini to
 * compile time types that ensure compile time safety.
 * Please check SampleGameplayTags.h for an example of how to declare literal native gameplay tags.
 *
 * The actual gameplay tags can be obtained like this:
 *
 *		FGameplayTag FooTag = FSampleGameplayTags::Foo::Get();
 *		FGameplayTag BetaTag = FSampleGameplayTags::Foo::Bar::Beta::Get();
 *
 * As you can see, these code tags use the C++ scope-resolution operator '::' instead of the dot '.',
 * which has the advantage that you don't need any instances when getting tags.
 *
 * The bool parameter of DECLARE_OUU_GAMEPLAY_TAGS determines whether the tags are added as native tags.
 * If yes, this has the advantage that you don't have to manually sync them with entries from a gameplay tags ini file.
 * If no, you might have a bit more runtime flexibility.
 */

/** Base class for the root of literal gameplay tag type hierarchies. */
OUURUNTIME_API struct FLiteralGameplayTagRoot
{
	// These members of this struct are just required so derived types can act as an
	// outer/parent tag type to the derived types of TLiteralGameplayTag.
protected:
	template <typename, typename, typename>
	friend struct TLiteralGameplayTag;

	static FString GetName() { return FString(""); }
};

template <typename ActualLiteralGameplayTagRootType, bool bInAutoAddNativeTag>
struct TLiteralGameplayTagRoot : FLiteralGameplayTagRoot
{
protected:
	using SelfTagType = ActualLiteralGameplayTagRootType;
	using RootTagType = ActualLiteralGameplayTagRootType;
	static const bool bAutoAddNativeTag = bInAutoAddNativeTag;

	template <typename, typename, typename>
	friend struct TLiteralGameplayTag;
};

struct CHasRelativeTagName
{
	template <typename T>
	auto Requires() -> decltype(FString(T::GetRelativeName()));
};

// Forward declaration of TLiteralGameplayTag template to be usable in CIsLiteralGameplayTag
template <typename InSelfTagType, typename InParentTagType, typename RootTagType>
struct TLiteralGameplayTag;

/** Concept to detect literal gameplay tags */
struct CIsLiteralGameplayTag
{
	template <typename A, typename B, typename C>
	void CheckBase(TLiteralGameplayTag<A, B, C>&&);

	template <typename T>
	auto Requires() -> decltype(CheckBase(DeclVal<T>()));
};

/** Base class for literal gameplay tags. Used for inheritance checks, but nothing else. */
class FLiteralGameplayTagBase
{
protected:
	virtual ~FLiteralGameplayTagBase() = default;
};

/**
 * The base class for literal gameplay tag types.
 * Derived types should be declared with the GTAG and GTAG_GROUP macros defined below.1
 */
template <typename InSelfTagType, typename InParentTagType, typename InRootTagType>
struct TLiteralGameplayTag : FLiteralGameplayTagBase
{
	using RootTagType = InRootTagType;
	using ThisType = TLiteralGameplayTag<InSelfTagType, InParentTagType, RootTagType>;
	using SelfTagType = InSelfTagType;
	using ParentTagType = InParentTagType;

	static_assert(
		TIsDerivedFrom<ParentTagType, FLiteralGameplayTagRoot>::Value
			|| TModels<CIsLiteralGameplayTag, ParentTagType>::Value,
		"ParentTagType must be either derived from FLiteralGameplayTagRoot or from LiteralGameplayTag");

	static_assert(
		TIsDerivedFrom<RootTagType, FLiteralGameplayTagRoot>::Value,
		"All literal gameplay tags must start with a root type that is derived from FLiteralGameplayTagRoot");

	static FString GetName()
	{
		static_assert(
			TModels<CHasRelativeTagName, InSelfTagType>::Value,
			"SelfTagType must have member GetRelativeName().\n"
			"It's strongly recommended to only create these derived types via the GTAG and GTAG_GROUP macros.");

		static_assert(
			TIsDerivedFrom<SelfTagType, ThisType>::Value && !TIsSame<ThisType, SelfTagType>::Value,
			"InSelfTagType must be a struct type derived from TLiteralGameplayTag, but not TLiteralGameplayTag "
			"itself.\n"
			"It's strongly recommended to only create these derived types via the GTAG and GTAG_GROUP macros.");

		return (ParentTagType::GetName().Len() > 0)
			? (ParentTagType::GetName() + FString(".") + SelfTagType::GetRelativeName())
			: SelfTagType::GetRelativeName();
	}
};

template <typename OwningType>
struct TConditionalNativeTag_Base
{
	static FGameplayTag Get(bool bErrorIfNotFound = true)
	{
		static FGameplayTag Tag =
			UGameplayTagsManager::Get().RequestGameplayTag(*OwningType::GetName(), bErrorIfNotFound);
		return Tag;
	}
};

template <typename OwningType, bool bAutoAddNativeTag>
struct TConditionalNativeTag : public TConditionalNativeTag_Base<OwningType>
{
};

#if WITH_NATIVE_GAMEPLAY_TAGS

template <typename OwningType>
struct TConditionalNativeTag<OwningType, true>
{
	FNativeGameplayTag NativeInstance{
		OwningType::GetPluginName(),
		OwningType::GetModuleName(),
		*OwningType::GetName(),
		OwningType::GetDescription(),
		ENativeGameplayTagToken::PRIVATE_USE_MACRO_INSTEAD};

	static FGameplayTag Get() { return OwningType::GetInstance().NativeInstance.GetTag(); }
};

#else

template <typename OwningType>
struct TConditionalNativeTag<OwningType, true> : public FGameplayTagNativeAdder
{
	FGameplayTag NativeInstance;

	virtual ~TConditionalNativeTag() {}

	virtual void AddTags() override
	{
		auto& GameplayTagsManager = UGameplayTagsManager::Get();
		NativeInstance = GameplayTagsManager.AddNativeGameplayTag(*OwningType::GetName(), OwningType::GetDescription());
	}

	static FGameplayTag Get(bool bErrorIfNotFound = true) { return OwningType::GetInstance().NativeInstance; }
};

#endif

/**
 * The root of a literal gameplay tag collection.
 * contains some settings that apply to all contained tags, e.g.
 * bAutoAddNativeTag: If yes, all the tags inside are added as native tags to the gameplay tags manager similar to how
 * FAutoConsoleObject registers with IConsoleManager.
 */
#define DECLARE_OUU_GAMEPLAY_TAGS(RootName, bAutoAddNativeTag, MODULE_API)                                             \
	struct RootName;                                                                                                   \
	extern RootName RootName##_Instance;                                                                               \
	struct MODULE_API RootName : TLiteralGameplayTagRoot<RootName, bAutoAddNativeTag>                                  \
	{                                                                                                                  \
		static RootName& GetInstance() { return RootName##_Instance; }

// clang-format off
#define DECLARE_OUU_GAMEPLAY_TAGS_END };
// clang-format on

/**
 * Put this into a cpp file so the literal gameplay tag root container is only defined once for multiple translation
 * units. This prevents linker errors due to multiple definitions.
 */
#define DEFINE_OUU_GAMEPLAY_TAGS(RootName) RootName RootName##_Instance;

/**
 * Create a tag that contains other tags. You have to close the group with OUU_GTAG_GROUP_END
 * Example:
 *    OUU_GTAG_GROUP(Foo)
 *        OUU_GTAG(Bar);
 *        OUU_GTAG(FooBar);
 *    OUU_GTAG_GROUP_END(Foo)
 */
#define OUU_GTAG_GROUP_START(TagName, TagDescription)                                                                  \
	OUU_LITERAL_GTAG_START_IMPL(TagName, PREPROCESSOR_TO_STRING(TagName), TagDescription)

/** Close a previously opened tag group. */
#define OUU_GTAG_GROUP_END(TagName)                                                                                    \
	}                                                                                                                  \
	TagName##_Instance;

/**
 * Create a leaf tag that does not contain other tags in the literal tags hierarchy in C++.
 * The "real" tag equivalent configured via GameplayTags.ini may contain sub-tags,
 * but those sub-tags won't be available via literal gameplay tags then.
 */
#define OUU_GTAG(TagName, TagDescription)                                                                              \
	OUU_LITERAL_GTAG_START_IMPL(TagName, PREPROCESSOR_TO_STRING(TagName), TagDescription)                              \
	}                                                                                                                  \
	TagName##_Instance;

#if WITH_NATIVE_GAMEPLAY_TAGS

	// Internal macro for engine versions with native gameplay tags
	#define OUU_LITERAL_GTAG_START_IMPL(Tag, TagNameString, TagDescription)                                            \
		struct Tag :                                                                                                   \
			TLiteralGameplayTag<Tag, SelfTagType, RootTagType>,                                                        \
			TConditionalNativeTag<Tag, RootTagType::bAutoAddNativeTag>                                                 \
		{                                                                                                              \
		public:                                                                                                        \
			static FString GetRelativeName() { return FString(TagNameString); }                                        \
			static FString GetDescription() { return FString(TEXT(TagDescription)); }                                  \
			static FName GetModuleName() { return UE_MODULE_NAME; }                                                    \
			static FName GetPluginName() { return UE_PLUGIN_NAME; }                                                    \
			static Tag& GetInstance() { return ParentTagType::GetInstance().##Tag##_Instance; }

#else

	// Internal macro for engine versions without native gameplay tags
	#define OUU_LITERAL_GTAG_START_IMPL(Tag, TagNameString, TagDescription)                                            \
		struct Tag :                                                                                                   \
			TLiteralGameplayTag<Tag, SelfTagType, RootTagType>,                                                        \
			TConditionalNativeTag<Tag, RootTagType::bAutoAddNativeTag>                                                 \
		{                                                                                                              \
		public:                                                                                                        \
			static FString GetRelativeName() { return FString(TagNameString); }                                        \
			static FString GetDescription() { return FString(TEXT(TagDescription)); }                                  \
			static Tag& GetInstance() { return ParentTagType::GetInstance().##Tag##_Instance; }

#endif
