// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "GameplayTagContainer.h"
#include "GameplayTagsManager.h"
#include "GameplayTagsModule.h"
#include "LiteralGameplayTagMacros.h"
#include "LogOpenUnrealUtilities.h"
#include "Misc/EngineVersionComparison.h"
#include "NativeGameplayTags.h"
#include "Templates/BitmaskUtils.h"
#include "Templates/UnrealTypeTraits.h"
#include "Traits/IsSameWrapper.h"

/**
 * --------------------------
 *  Literal (Native) Gameplay Tags
 * --------------------------
 *
 * Literal tags offer the possibility to map gameplay tags defined in DefaultGameplayTags.ini to
 * compile time types that ensure compile time safety. Literal tags are meant to be used in conjunction with
 * TypedGameplayTags, which allow exposing "typesafe" tags to properties & blueprint.
 *
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
 * The bool parameter of OUU_DECLARE_GAMEPLAY_TAGS_START determines whether the tags are added as native tags.
 * If yes, this has the advantage that you don't have to manually sync them with entries from a gameplay tags ini file.
 * If no, you might have a bit more runtime flexibility.
 */
//---------------------------------------------------------------------------------------------------------------------

template <typename Child, typename TestParent>
struct TIsChildTagOf_Single
{
	static constexpr bool Value =
		TOr<TIsSameWrapper<typename Child::SelfTagType, TestParent>,
			TAnd<
				TNot<TIsSameWrapper<typename Child::SelfTagType, typename Child::ParentTagType>>,
				TIsChildTagOf_Single<typename Child::ParentTagType, TestParent>>>::Value;
};

template <typename Child, typename... TestParents>
struct TIsChildTagOf
{
	static constexpr bool Value = TOr<TIsChildTagOf_Single<Child, TestParents>...>::Value;
};

enum class ELiteralGameplayTagFlags
{
	None = 0,

	// Automatically register the tags.
	// Should be omitted, if the tag declarations are referring to tags registered elsewhere.
	AutoRegister = 0b1,

	// Automatically set up an entry in GameplayTagValidationSettings to allow child tags to be created.
	AllowContentChildTags = 0b10,

	// These tag flags are inherited from the parent tag.
	Inherited = 0b100,

	// Recommended default tags for most use-cases
	Default = (AutoRegister)
};

DECLARE_BITMASK_OPERATORS(ELiteralGameplayTagFlags)

constexpr ELiteralGameplayTagFlags AutoConvertLiteralGameplayTagFlags(ELiteralGameplayTagFlags Flags)
{
	return Flags;
}

UE_DEPRECATED(
	5.2,
	"Using bool in literal gameplay tag declarations is deprecated. Please use ELiteralGameplayTagFlags instead.")
constexpr ELiteralGameplayTagFlags AutoConvertLiteralGameplayTagFlags(bool bInAutoAddNativeTag)
{
	return bInAutoAddNativeTag ? ELiteralGameplayTagFlags::AutoRegister : ELiteralGameplayTagFlags::None;
}

/**
 * The base class for literal gameplay tag types.
 * Derived types should be declared with the GTAG and GTAG_GROUP macros defined below.
 */
template <typename InSelfTagType, typename InParentTagType, typename InRootLiteralTagType>
struct TLiteralGameplayTag
{
	using SelfTagType = InSelfTagType;
	using ParentTagType = InParentTagType;
	using RootTagType = InRootLiteralTagType;
	using ThisType = TLiteralGameplayTag<SelfTagType, ParentTagType, RootTagType>;

#define RESULT_IF_ROOT_TAG(Result, Expected)                                                                           \
	template <typename T = SelfTagType>                                                                                \
	static typename TEnableIf<TIsSame<RootTagType, T>::Value == Expected, Result>::Type

	// Return the relative name as full name if this is the root tag
	RESULT_IF_ROOT_TAG(FString, true) GetName() { return SelfTagType::GetRelativeName(); }

	// Return the parent name plus the relative name as full name if this is not the root tag
	RESULT_IF_ROOT_TAG(FString, false) GetName()
	{
		AssertIsProperTagType<SelfTagType>();
		AssertIsProperTagType<ParentTagType>();
		AssertIsProperTagType<RootTagType>();

		return ParentTagType::GetName() + FString(".") + SelfTagType::GetRelativeName();
	}

#undef RESULT_IF_ROOT_TAG

	static const SelfTagType& Get() { return SelfTagType::GetInstance(); }
	static const FGameplayTag GetTag() { return SelfTagType::GetInstance().NativeTagGetter.Get(); }

	operator FGameplayTag() const { return GetTag(); }

private:
	struct CHasRelativeTagName
	{
		template <typename T>
		auto Requires() -> decltype(FString(T::GetRelativeName()));
	};

	template <typename T>
	static void AssertIsProperTagType()
	{
		using TemplateType =
			TLiteralGameplayTag<typename T::SelfTagType, typename T::ParentTagType, typename T::RootTagType>;
		static_assert(
			TIsDerivedFrom<T, TemplateType>::Value && !std::is_same_v<T, TemplateType>,
			"Type must be a struct type derived from TLiteralGameplayTag, but not TLiteralGameplayTag "
			"itself.\n"
			"It's strongly recommended to only create these derived types via the GTAG and GTAG_GROUP macros.");

		// we only check for this one function.
		static_assert(
			TModels<CHasRelativeTagName, T>::Value,
			"Type must have member GetRelativeName().\n"
			"It's strongly recommended to only create these derived types via the GTAG and GTAG_GROUP macros.");
	}
};

template <typename SelfTagType, typename ParentTagType, typename RootLiteralTagType>
bool operator==(const TLiteralGameplayTag<SelfTagType, ParentTagType, RootLiteralTagType>& LHS, const FGameplayTag& RHS)
{
	return LHS.GetTag() == RHS;
}

template <typename SelfTagType, typename ParentTagType, typename RootLiteralTagType>
bool operator==(const FGameplayTag& LHS, const TLiteralGameplayTag<SelfTagType, ParentTagType, RootLiteralTagType>& RHS)
{
	return RHS == LHS;
}

//---------------------------------------------------------------------------------------------------------------------

/**
 * The root of a group of literal gameplay tags.
 *
 * ELiteralGameplayTagFlags can be used as single optional last parameter. ELiteralGameplayTagFlags::Default if omitted.
 * Flags propagate to child tags unless overridden.
 */
#define OUU_DECLARE_GAMEPLAY_TAGS_START(MODULE_API, TagType, RootTagName, Description, ...)                            \
	OUU_DECLARE_GAMEPLAY_TAGS_START_IMPL(                                                                              \
		MODULE_API,                                                                                                    \
		TagType,                                                                                                       \
		RootTagName,                                                                                                   \
		Description,                                                                                                   \
		RESOLVE_OPTIONAL_FLAGS(, ##__VA_ARGS__, __VA_ARGS__, ELiteralGameplayTagFlags::Default))

/**
 * Always use this at the end of a group of literal gameplay tag declarations.
 */
#define OUU_DECLARE_GAMEPLAY_TAGS_END(TypeName) OUU_DECLARE_GAMEPLAY_TAGS_END_IMPL(TypeName)

/**
 * Put this into a cpp file so the literal gameplay tags are only defined once for multiple translation units.
 * This prevents linker errors due to multiple definitions.
 */
#define OUU_DEFINE_GAMEPLAY_TAGS(TagType) TagType TagType##_Instance;

/**
 * Create a tag that contains other tags. You have to close the group with OUU_GTAG_GROUP_END
 * Example:
 *    OUU_GTAG_GROUP_START(Foo)
 *        OUU_GTAG(Bar);
 *        OUU_GTAG(FooBar);
 *    OUU_GTAG_GROUP_END(Foo)
 *
 * ELiteralGameplayTagFlags can be used as single optional last parameter. ELiteralGameplayTagFlags::Default if omitted.
 * Flags propagate to child tags unless overridden.
 */
#define OUU_GTAG_GROUP_START(TagType, TagDescription, ...)                                                             \
	OUU_GTAG_GROUP_START_IMPL(                                                                                         \
		TagType,                                                                                                       \
		TagDescription,                                                                                                \
		RESOLVE_OPTIONAL_FLAGS(, #__VA_ARGS__, __VA_ARGS__, (Flags & ELiteralGameplayTagFlags::Inherited)))

/** Close a previously opened tag group. */
// clang-format off
#define OUU_GTAG_GROUP_END(TagType)                                                                                    \
	OUU_GTAG_GROUP_END_IMPL(TagType)
// clang-format on

/**
 * Create a leaf tag that does not contain other tags in the literal tags hierarchy in C++.
 * The "real" tag equivalent configured via GameplayTags.ini may contain sub-tags,
 * but those sub-tags won't be available via literal gameplay tags then.
 *
 * ELiteralGameplayTagFlags can be used as single optional last parameter. ELiteralGameplayTagFlags::Default if omitted.
 * Flags propagate to child tags unless overridden.
 */
#define OUU_GTAG(Tag, TagDescription, ...)                                                                             \
	OUU_GTAG_GROUP_START(Tag, TagDescription, #__VA_ARGS__)                                                            \
	OUU_GTAG_GROUP_END(Tag)

//---------------------------------------------------------------------------------------------------------------------

namespace OUU::Editor::Private
{
#if WITH_EDITOR
	OUURUNTIME_API void RegisterLiteralTagFlagsForValidation(FGameplayTag Tag, ELiteralGameplayTagFlags Flags);
	OUURUNTIME_API const TMap<FGameplayTag, ELiteralGameplayTagFlags>& GetTagFlagsForValidation();
#endif
} // namespace OUU::Editor::Private

namespace OUU::Runtime::Private
{
	template <typename OwningType>
	struct TConditionalNativeTagGetter_Base
	{
		static FGameplayTag Get()
		{
			// Never use the ensure, display a custom error instead if the tag was not found.
			// That way the API stays consistent without bool parameter.
			const bool bErrorIfNotFound = false;
			static FGameplayTag Tag =
				UGameplayTagsManager::Get().RequestGameplayTag(*OwningType::GetName(), bErrorIfNotFound);
			UE_CLOG(
				Tag == Tag.EmptyTag,
				LogOpenUnrealUtilities,
				Error,
				TEXT("Requested Gameplay Tag %s was not found, tags must be loaded from config or registered as a "
					 "native "
					 "tag"),
				*OwningType::GetName());
			return Tag;
		}
	};

	template <typename OwningType, ELiteralGameplayTagFlags Flags, bool bAutoAddNativeTag>
	struct TConditionalNativeTagGetter : public TConditionalNativeTagGetter_Base<OwningType>
	{
	};

	template <typename OwningType, ELiteralGameplayTagFlags Flags>
	struct TConditionalNativeTagGetter<OwningType, Flags, true>
	{
	public:
		TConditionalNativeTagGetter()
		{
#if WITH_EDITOR
			OUU::Editor::Private::RegisterLiteralTagFlagsForValidation(Get(), Flags);
#endif
		}

	private:
		FNativeGameplayTag NativeInstance{
			OwningType::GetPluginName(),
			OwningType::GetModuleName(),
			*OwningType::GetName(),
			OwningType::GetDescription(),
			ENativeGameplayTagToken::PRIVATE_USE_MACRO_INSTEAD};

	public:
		static FGameplayTag Get() { return OwningType::GetInstance().NativeTagGetter.NativeInstance.GetTag(); }
	};
} // namespace OUU::Runtime::Private
