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
#include "Traits/AssertValueEquality.h"
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

// can be used as EFlag in macros
enum class ELiteralGameplayTagFlags
{
	None = 0,

	// Automatically register the tags.
	// Should be omitted, if the tag declarations are referring to tags registered elsewhere.
	AutoRegister = 0b1,

	// Automatically set up an entry in GameplayTagValidationSettings to allow child tags to be created.
	AllowContentChildTags = 0b10,

	// This tag is automatically added to a flag set that is otherwise 100% inherited from the parent tag.
	// When used in the macros it's automatically removed if you explicitly override tags.
	Inherited = 0b100,

	// These flags were set explicitly. If the flag parameter was left empty, this flag won't be set.
	Explicit = 0b1000,

	// Recommended default tags for most use-cases
	Default = (AutoRegister),

	// For backwards compatibility: true and false in macros should be converted to these bitmasks respectively:
	Legacy_True = Default,
	Legacy_False = None
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
	return bInAutoAddNativeTag ? ELiteralGameplayTagFlags::Legacy_True : ELiteralGameplayTagFlags::Legacy_False;
}

// Use this like so in macros: ResolveFallbackFlags(Fallback, ##__VA_ARGS__)
// If no VA_ARGS are passed, the compiler uses the somgöe parameter version below.
// Otherwise it uses this version, which invokes one of the funcs above with the SECOND parameter, which needs to be the
// explicit flags.
template <typename T, typename U>
constexpr ELiteralGameplayTagFlags ResolveFallbackFlags(T ImplicitFallbackFlags, U ExplicitFlags)
{
	return (AutoConvertLiteralGameplayTagFlags(ExplicitFlags)
			// Guaranteed explicit flags
			| ELiteralGameplayTagFlags::Explicit)
		// -> Guaranteed not inherited
		& (~ELiteralGameplayTagFlags::Inherited);
}

template <typename T>
constexpr ELiteralGameplayTagFlags ResolveFallbackFlags(T ImplicitFallbackFlags)
{
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	return AutoConvertLiteralGameplayTagFlags(ImplicitFallbackFlags)
		// Guaranteed implicit flags -> not explicit
		& (~ELiteralGameplayTagFlags::Explicit)
		// Could be new (if on root level) or inherited.
		// That's why we can't set Inherited here
		;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
}

/**
 * The base class for literal gameplay tag types.
 * Derived types should be declared with the GTAG and GGROUP macros defined below.
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
			"It's strongly recommended to only create these derived types via the GTAG and GGROUP macros.");

		// we only check for this one function.
		static_assert(
			TModels<CHasRelativeTagName, T>::Value,
			"Type must have member GetRelativeName().\n"
			"It's strongly recommended to only create these derived types via the GTAG and GGROUP macros.");
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
		(ResolveFallbackFlags(ELiteralGameplayTagFlags::Default, ##__VA_ARGS__)))

/**
 * Use this if you want to extend an existing literal gameplay tag declaration from another module.
 */
#define OUU_DECLARE_GAMEPLAY_TAGS_EXTENSION_START(MODULE_API, TagType, TagTypeToExtend)                                \
	struct TagType;                                                                                                    \
	extern TagType TagType##_Instance;                                                                                 \
	struct MODULE_API TagType : public TLiteralGameplayTag<TagType, TagType, TagType>                                  \
	{                                                                                                                  \
		using EFlags = ELiteralGameplayTagFlags;                                                                       \
                                                                                                                       \
	public:                                                                                                            \
		static const ELiteralGameplayTagFlags Flags = TagTypeToExtend::Flags;                                          \
		/* do not auto-register the tag via extension. should only ever be done from a "normal" declaration. */        \
		static const bool bAutoAddNativeTag = false;                                                                   \
		static const TagType& GetInstance() { return TagType##_Instance; }                                             \
		/* Return the literal tag this is extending to allow existing implicit conversion to e.g. typed tags */        \
		static const TagTypeToExtend& Get() { return TagTypeToExtend::Get(); }                                         \
                                                                                                                       \
	public:                                                                                                            \
		static FString GetRelativeName() { return TagTypeToExtend::GetRelativeName(); }                                \
		static FString GetDescription() { return TagTypeToExtend::GetDescription(); }                                  \
		static FName GetModuleName();                                                                                  \
		static FName GetPluginName();                                                                                  \
                                                                                                                       \
		PRIVATE_OUU_GTAG_GETTER_IMPL(TagType, TagType)                                                                 \
	public:

/**
 * Always use this at the end of a group of literal gameplay tag declarations.
 */
#define OUU_DECLARE_GAMEPLAY_TAGS_END(TypeName) OUU_DECLARE_GAMEPLAY_TAGS_END_IMPL(TypeName)

/**
 * Put this into a cpp file so the literal gameplay tags are only defined once for multiple translation units.
 * This prevents linker errors due to multiple definitions.
 */
#define OUU_DEFINE_GAMEPLAY_TAGS(TagType)                                                                              \
	TagType TagType##_Instance;                                                                                        \
	FName TagType::GetModuleName() { return UE_MODULE_NAME; }                                                          \
	FName TagType::GetPluginName() { return UE_PLUGIN_NAME; }

/**
 * Create a tag that contains other tags. You have to close the group with OUU_GGROUP_END
 * Example:
 *    OUU_GGROUP_START(Foo)
 *        OUU_GTAG(Bar);
 *        OUU_GTAG(FooBar);
 *    OUU_GGROUP_END(Foo)
 *
 * ELiteralGameplayTagFlags can be used as single optional last parameter. ELiteralGameplayTagFlags::Default if
 * omitted. Flags propagate to child tags unless overridden.
 */
#define OUU_GTAG_GROUP_START(TagType, TagDescription, ...)                                                             \
	OUU_GTAG_GROUP_START_IMPL(                                                                                         \
		TagType,                                                                                                       \
		TagDescription,                                                                                                \
		(ResolveFallbackFlags((Flags & ELiteralGameplayTagFlags::Inherited)), ##__VA_ARGS__))

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
 * ELiteralGameplayTagFlags can be used as single optional last parameter. ELiteralGameplayTagFlags::Default if
 * omitted. Flags propagate to child tags unless overridden.
 */
#define OUU_GTAG(Tag, TagDescription, ...)                                                                             \
	OUU_GTAG_GROUP_START(Tag, TagDescription, ##__VA_ARGS__)                                                           \
	OUU_GTAG_GROUP_END(Tag)

//---------------------------------------------------------------------------------------------------------------------

namespace OUU::Editor::Private
{
#if WITH_EDITOR
	OUURUNTIME_API void RegisterLiteralTagFlagsForValidation(
		FGameplayTag Tag,
		ELiteralGameplayTagFlags Flags,
		FName PluginName,
		FName ModuleName);
	OUURUNTIME_API const TMap<FGameplayTag, ELiteralGameplayTagFlags>& GetTagFlagsForValidation();
#endif
} // namespace OUU::Editor::Private

namespace OUU::Runtime::Private
{
	template <typename OwningType>
	struct TConditionalNativeTagGetter_Base
	{
	protected:
		static auto RequestTag()
		{
			// Never use the ensure, display a custom error instead if the tag was not found.
			// That way the API stays consistent without bool parameter.
			const bool bErrorIfNotFound = false;
			return UGameplayTagsManager::Get().RequestGameplayTag(*OwningType::GetName(), bErrorIfNotFound);
		}
	};

	template <typename OwningType, ELiteralGameplayTagFlags Flags, bool bAutoAddNativeTag>
	struct TConditionalNativeTagGetter : public TConditionalNativeTagGetter_Base<OwningType>
	{
	public:
		static FGameplayTag Get()
		{
			static FGameplayTag Tag = RequestTag();
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

	template <typename OwningType, ELiteralGameplayTagFlags Flags>
	struct TConditionalNativeTagGetter<OwningType, Flags, true> : public TConditionalNativeTagGetter_Base<OwningType>
	{
	public:
		TConditionalNativeTagGetter()
		{
#if WITH_EDITOR
			OUU::Editor::Private::RegisterLiteralTagFlagsForValidation(
				Get(),
				Flags,
				OwningType::RootTagType::GetPluginName(),
				OwningType::RootTagType::GetModuleName());
#endif
		}

	private:
		FNativeGameplayTag NativeInstance{
			OwningType::RootTagType::GetPluginName(),
			OwningType::RootTagType::GetModuleName(),
			*OwningType::GetName(),
			OwningType::GetDescription(),
			ENativeGameplayTagToken::PRIVATE_USE_MACRO_INSTEAD};

	public:
		static FGameplayTag Get() { return OwningType::GetInstance().NativeTagGetter.NativeInstance.GetTag(); }
	};
} // namespace OUU::Runtime::Private

//---------------------------------------------------------------------------------------------------------------------

// Test that ResolveFallbackFlags function works as expected
namespace OUU::Runtime::Private::ResolveFallbackFlagsTests
{
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	static_assert(
		ResolveFallbackFlags(true) == (ELiteralGameplayTagFlags::Legacy_True),
		"true should be converted to Legacy_True");

	static_assert(
		ResolveFallbackFlags(false) == (ELiteralGameplayTagFlags::Legacy_False),
		"false should be converted to Legacy_False");

	static_assert(
		ResolveFallbackFlags(ELiteralGameplayTagFlags::AllowContentChildTags)
			== ELiteralGameplayTagFlags::AllowContentChildTags,
		"fallback should be passed");

	static_assert(
		ResolveFallbackFlags(ELiteralGameplayTagFlags::None, ELiteralGameplayTagFlags::AllowContentChildTags)
			== (ELiteralGameplayTagFlags::AllowContentChildTags | ELiteralGameplayTagFlags::Explicit),
		"explicit flags should be passed and Explicit should be added");

	static_assert(
		ResolveFallbackFlags(ELiteralGameplayTagFlags::Default | ELiteralGameplayTagFlags::Inherited)
			== (ELiteralGameplayTagFlags::Default | ELiteralGameplayTagFlags::Inherited),
		"Inherited tag should be kept");

	constexpr ELiteralGameplayTagFlags ActualFlags = ResolveFallbackFlags(
		"this string should lead to a compile error if ever considered",
		(ELiteralGameplayTagFlags::Default | ELiteralGameplayTagFlags::Inherited
		 | ELiteralGameplayTagFlags::AllowContentChildTags));
	constexpr ELiteralGameplayTagFlags ExpectedFlags = ELiteralGameplayTagFlags(
		ELiteralGameplayTagFlags::Default | ELiteralGameplayTagFlags::AllowContentChildTags
		| ELiteralGameplayTagFlags::Explicit);
	static_assert(
		TAssertValuesEqual<ELiteralGameplayTagFlags, ActualFlags, ExpectedFlags>::Value,
		"Inherited tag should be removed + Explicit should be added");
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
} // namespace OUU::Runtime::Private::ResolveFallbackFlagsTests
