// Copyright (c) 2022 Jonas Reich

#pragma once

#include "CoreMinimal.h"

#include "GameplayTags/LiteralGameplayTag.h"

//---------------------------------------------------------------------------------------------------------------------

namespace OUU::Runtime::Private
{
	struct OUURUNTIME_API FTypedGameplayTag_Base
	{
#if WITH_EDITOR
		static void RegisterPropertTypeLayout(const FString& TypeName);
		static void UnregisterPropertTypeLayout(const FString& TypeName);
#endif
	};
} // namespace OUU::Runtime::Private

/**
 * Base class for blueprint/property exposed typesafe tags.
 * Typesafe in the sense that you can only assign child tags of the root tag.
 * Supports assignment and comparison with literal gameplay tags.
 *
 * Inspired by TTypedTagStaticImpl / FUITag
 *
 * Usage:
 *    - Subclass this and FGameplayTag
 *    - Assign Categories meta specifier to matching tag
 *    - use DEFINE_TYPED_GAMEPLAY_TAG macro in body.
 * See FOUUSampleTag.
 *
 * Please note that for property editors to work correctly, you need to take care of property type layout customization
 * in your module startup/shutdown by calling
 * - FYourTagType::RegisterCustomProperyTypeLayout()
 * - FYourTagType::UnregisterCustomProperyTypeLayout()
 * respectively.
 */
template <typename InBlueprintTagType, typename InRootLiteralTagType>
struct TTypedGameplayTag : public OUU::Runtime::Private::FTypedGameplayTag_Base
{
public:
	using BlueprintTagType = InBlueprintTagType;
	using RootLiteralTagType = InRootLiteralTagType;
	using TypedGameplayTagType = TTypedGameplayTag<BlueprintTagType, RootLiteralTagType>;

	friend BlueprintTagType;

	TTypedGameplayTag(BlueprintTagType& InOwningTag) : OwningTag(InOwningTag) {}
	TTypedGameplayTag(const TTypedGameplayTag&) = default;

	template <typename T, typename U, typename V>
	BlueprintTagType& operator=(const TLiteralGameplayTag<T, U, V>& LiteralGameplayTag)
	{
		AssertLiteralGameplayTag(LiteralGameplayTag);
		OwningTag = LiteralGameplayTag.GetTag();
		return OwningTag;
	}

	TTypedGameplayTag& operator=(const TTypedGameplayTag&) = default;

private:
	BlueprintTagType& OwningTag;

	template <typename T, typename U, typename V>
	constexpr bool AssertLiteralGameplayTag(const TLiteralGameplayTag<T, U, V>& LiteralGameplayTag) const
	{
		using ParamTagType = TLiteralGameplayTag<T, U, V>;

		static_assert(
			TIsDerivedFrom<BlueprintTagType, FGameplayTag>::Value,
			"BlueprintStructType must be derived from FGameplayTag");

		static_assert(
			TIsChildTagOf<ParamTagType, RootLiteralTagType>::Value,
			"Can only assign from a literal gameplay tag that is nested under RootTagType");

		return true;
	}

	static BlueprintTagType TryConvert(FGameplayTag VanillaTag, bool bChecked)
	{
		FGameplayTag RootTag = InRootLiteralTagType::Get();

		if (VanillaTag.MatchesTag(RootTag))
		{
			return BlueprintTagType(VanillaTag);
		}
		else if (VanillaTag.IsValid() && bChecked)
		{
			check(false);
		}
		return BlueprintTagType();
	}
};

/**
 * Use in derived types of TLiteralGameplayTagInstance to inherit the literal gameplay tag functionality.
 * Does not support usage together with custom constructors!
 */
#define DEFINE_TYPED_GAMEPLAY_TAG(TagType, InLiteralGameplayTagType)                                                   \
	PRIVATE_TYPED_GAMEPLAY_TAG_IMPL(TagType, InLiteralGameplayTagType)

//---------------------------------------------------------------------------------------------------------------------

#define PRIVATE_TYPED_GAMEPLAY_TAG_IMPL(TagType, InLiteralGameplayTagType)                                             \
public:                                                                                                                \
	using LiteralGameplayTagType = InLiteralGameplayTagType;                                                           \
	using TypedTagImplType = TTypedGameplayTag<TagType, LiteralGameplayTagType>;                                       \
	TagType() = default;                                                                                               \
                                                                                                                       \
	template <typename T, typename U, typename V>                                                                      \
	TagType(const TLiteralGameplayTag<T, U, V>& LiteralGameplayTag)                                                    \
	{                                                                                                                  \
		using ParamTagType = TLiteralGameplayTag<T, U, V>;                                                             \
		static_assert(                                                                                                 \
			TIsChildTagOf<T, LiteralGameplayTagType>::Value,                                                           \
			"Can only assign from a literal gameplay tag that is nested under " PREPROCESSOR_TO_STRING(                \
				LiteralGameplayTagType));                                                                              \
		TypedTag_Impl = LiteralGameplayTag;                                                                            \
	}                                                                                                                  \
	TagType& operator=(const TagType& Other)                                                                           \
	{                                                                                                                  \
		static_cast<FGameplayTag&>(*this) = static_cast<const FGameplayTag&>(Other);                                   \
		return *this;                                                                                                  \
	}                                                                                                                  \
	static TagType TryConvert(FGameplayTag FromTag) { return TypedTagImplType::TryConvert(FromTag, false); }           \
	static TagType ConvertChecked(FGameplayTag FromTag) { return TypedTagImplType::TryConvert(FromTag, true); }        \
                                                                                                                       \
	PRIVATE_TYPED_GAMEPLAY_TAG_EDITOR_IMPL(TagType)                                                                    \
                                                                                                                       \
private:                                                                                                               \
	TypedTagImplType TypedTag_Impl{*this};                                                                             \
                                                                                                                       \
	TagType(FGameplayTag Tag) { TagName = Tag.GetTagName(); }                                                          \
	template <typename, typename>                                                                                      \
	friend struct TTypedGameplayTag;

#if WITH_EDITOR
	#define PRIVATE_TYPED_GAMEPLAY_TAG_EDITOR_IMPL(TagType)                                                            \
		static void RegisterCustomProperyTypeLayout()                                                                  \
		{                                                                                                              \
			TypedTagImplType::RegisterPropertTypeLayout(PREPROCESSOR_TO_STRING(TagType));                              \
		}                                                                                                              \
		static void UnregisterCustomProperyTypeLayout()                                                                \
		{                                                                                                              \
			TypedTagImplType::UnregisterPropertTypeLayout(PREPROCESSOR_TO_STRING(TagType));                            \
		}
#else
	#define PRIVATE_TYPED_GAMEPLAY_TAG_EDITOR_IMPL(TagName) PREPROCESSOR_NOTHING
#endif

//---------------------------------------------------------------------------------------------------------------------

// Forward declare the derived types...

// 1) The reference type points to an external gameplay tag container and should be preferred.
template <typename BlueprintTagType>
struct TTypedGameplayTagContainerReference;

// 2) The value type contains the gameplay tag container as member and can e.g. be used for return values.
template <typename BlueprintTagType>
struct TTypedGameplayTagContainerValue;

// #TODO-jreich Move to private namespace

/**
 * Template wrapper for gameplay tag container that only allows assignment of matching gameplay tags and tag containers.
 */
template <typename InBlueprintTagType, typename DerivedImplementationType>
struct TTypedGameplayTagContainer_Base
{
public:
	using BlueprintTagType = InBlueprintTagType;
	using LiteralGameplayTagType = typename BlueprintTagType::LiteralGameplayTagType;

	using ValueContainerType = TTypedGameplayTagContainerValue<BlueprintTagType>;
	using ReferenceContainerType = TTypedGameplayTagContainerReference<BlueprintTagType>;

public:
	static FGameplayTagContainer FilterRootTag(const FGameplayTagContainer& RegularGameplayTags)
	{
		return RegularGameplayTags.Filter(FGameplayTagContainer(LiteralGameplayTagType::Get()));
	}

	// Call this in derived constructors / conversion functions.
	void EnsureValidRootTag() const
	{
#if DO_CHECK
		FGameplayTag RootTag = LiteralGameplayTagType::Get();
		const FGameplayTagContainer& ContainerRef = GetRef();
		for (const FGameplayTag& Tag : ContainerRef)
		{
			ensureMsgf(
				Tag.MatchesTag(RootTag),
				TEXT("Typed gameplay tag container contains tag '%s' that does not match the required root tag %s"),
				*Tag.ToString(),
				*RootTag.ToString());
		}
#endif
	}

private:
	FORCEINLINE FGameplayTagContainer& GetRef()
	{
		return static_cast<DerivedImplementationType*>(this)->DerivedImplementationType::GetRef_Impl();
	}
	FORCEINLINE const FGameplayTagContainer& GetRef() const
	{
		return static_cast<const DerivedImplementationType*>(this)->DerivedImplementationType::GetRef_Impl();
	}

public:
	// -- Functions mirroring the FGameplayTagContainer API:
#pragma region GameplayTagContainerAPI
	FORCEINLINE bool HasTag(const BlueprintTagType& TagToCheck) const { return GetRef().HasTag(TagToCheck); }
	FORCEINLINE bool HasTagExact(const BlueprintTagType& TagToCheck) const { return GetRef().HasTagExact(TagToCheck); }
	FORCEINLINE bool HasAny(const ReferenceContainerType ContainerToCheck) const
	{
		return GetRef().HasAny(ContainerToCheck.GetRef());
	}
	FORCEINLINE bool HasAnyExact(const ReferenceContainerType ContainerToCheck) const
	{
		return GetRef().HasAnyExact(ContainerToCheck.GetRef());
	}
	FORCEINLINE bool HasAll(const ReferenceContainerType ContainerToCheck) const
	{
		return GetRef().HasAll(ContainerToCheck.GetRef());
	}
	FORCEINLINE bool HasAllExact(const ReferenceContainerType ContainerToCheck) const
	{
		return GetRef().HasAllExact(ContainerToCheck.GetRef());
	}
	FORCEINLINE int32 Num() const { return GetRef().Num(); }
	FORCEINLINE bool IsValid() const { return GetRef().IsValid(); }
	FORCEINLINE bool IsEmpty() const { return GetRef().IsEmpty(); }
	FORCEINLINE FGameplayTagContainer GetGameplayTagParents() const
	{
		// Can't be sure that these are still underneath the root tag, so return a regular container
		return GetRef().GetGameplayTagParents();
	}
	FORCEINLINE ValueContainerType Filter(const ReferenceContainerType OtherContainer) const
	{
		return ValueContainerType::CreateChecked(GetRef().Filter(OtherContainer));
	}
	FORCEINLINE ValueContainerType FilterExact(const ReferenceContainerType OtherContainer) const
	{
		return ValueContainerType::CreateChecked(GetRef().FilterExact(OtherContainer));
	}
	bool MatchesQuery(const struct FGameplayTagQuery& Query) const { return GetRef().MatchesQuery(Query); }
	void AppendTags(const ReferenceContainerType& Other) { return GetRef().AppendTags(Other.GetRef()); }
	void AppendMatchingTags(ReferenceContainerType const& OtherA, ReferenceContainerType const& OtherB)
	{
		return GetRef().AppendMatchingTags(OtherA.GetRef(), OtherB.GetRef());
	}
	void AddTag(const BlueprintTagType& TagToAdd) { return GetRef().AddTag(TagToAdd); }
	void AddTagFast(const BlueprintTagType& TagToAdd) { return GetRef().AddTagFast(TagToAdd); }
	bool AddLeafTag(const BlueprintTagType& TagToAdd) { return GetRef().AddLeafTag(TagToAdd); }
	bool RemoveTag(const BlueprintTagType& TagToRemove, bool bDeferParentTags = false)
	{
		return GetRef().RemoveTag(TagToRemove, bDeferParentTags);
	}
	void RemoveTags(const ReferenceContainerType& TagsToRemove) { return GetRef().RemoveTags(TagsToRemove.GetRef()); }
	void Reset(int32 Slack = 0) { return GetRef().Reset(Slack); }
	FString ToString() const { return GetRef().ToString(); }
	FString ToStringSimple(bool bQuoted = false) const { return GetRef().ToStringSimple(bQuoted); }
	TArray<FString> ToStringsMaxLen(int32 MaxLen) const { return GetRef().ToStringsMaxLen(MaxLen); }
	FText ToMatchingText(EGameplayContainerMatchType MatchType, bool bInvertCondition) const
	{
		return GetRef().ToMatchingText(MatchType, bInvertCondition);
	}
	void GetGameplayTagArray(TArray<FGameplayTag>& InOutGameplayTags) const
	{
		return GetRef().GetGameplayTagArray(OUT InOutGameplayTags);
	}
	TArray<FGameplayTag>::TConstIterator CreateConstIterator() const { return GetRef().CreateConstIterator(); }
	bool IsValidIndex(int32 Index) const { return GetRef().IsValidIndex(); }
	BlueprintTagType GetByIndex(int32 Index) const { return GetRef().GetByIndex(Index); }
	BlueprintTagType First() const { return GetRef().First(); }
	BlueprintTagType Last() const { return GetRef().Last(); }
	void FillParentTags() { return GetRef().FillParentTags(); }
#pragma endregion GameplayTagContainerAPI
	// -- end of functions mirroring the FGameplayTagContainer API
};

//---------------------------------------------------------------------------------------------------------------------

template <typename InBlueprintTagType>
struct TTypedGameplayTagContainerReference :
	public TTypedGameplayTagContainer_Base<
		InBlueprintTagType,
		TTypedGameplayTagContainerReference<InBlueprintTagType>>
{
	template <typename, typename>
	friend struct TTypedGameplayTagContainer_Base;

	using BlueprintTagType = InBlueprintTagType;
	using LiteralGameplayTagType = typename BlueprintTagType::LiteralGameplayTagType;
	using SelfType = TTypedGameplayTagContainerReference<BlueprintTagType>;
	using Super = TTypedGameplayTagContainer_Base<BlueprintTagType, SelfType>;

public:
	TTypedGameplayTagContainerReference(FGameplayTagContainer& InGameplayTagContainerRef) :
		GameplayTagContainerRef(InGameplayTagContainerRef)
	{
		Super::EnsureValidRootTag();
	};

private:
	FGameplayTagContainer& GetRef_Impl() { return GameplayTagContainerRef; }
	const FGameplayTagContainer& GetRef_Impl() const { return GameplayTagContainerRef; }

private:
	FGameplayTagContainer& GameplayTagContainerRef;
};

//---------------------------------------------------------------------------------------------------------------------

template <typename InBlueprintTagType>
struct TTypedGameplayTagContainerValue :
	public TTypedGameplayTagContainer_Base<
		InBlueprintTagType,
		TTypedGameplayTagContainerValue<InBlueprintTagType>>
{
	template <typename, typename>
	friend struct TTypedGameplayTagContainer_Base;

	using BlueprintTagType = InBlueprintTagType;
	using LiteralGameplayTagType = typename BlueprintTagType::LiteralGameplayTagType;
	using SelfType = TTypedGameplayTagContainerValue<BlueprintTagType>;
	using Super = TTypedGameplayTagContainer_Base<BlueprintTagType, SelfType>;

public:
	TTypedGameplayTagContainerValue() = default;
	// explicit conversion from ref type (so we don't create accidental copies)
	explicit TTypedGameplayTagContainerValue(const TTypedGameplayTagContainerReference<BlueprintTagType>& Ref) :
		GameplayTagContainerValue(Ref.GetRef())
	{
	}
	// auto conversion to ref type
	operator TTypedGameplayTagContainerReference<BlueprintTagType>() const
	{
		return TTypedGameplayTagContainerReference<BlueprintTagType>(
			const_cast<SelfType*>(this)->GameplayTagContainerValue);
	}

	/** Create a contianer from another container that is assumed to be pre-filtered (faster). */
	static SelfType CreateChecked(const FGameplayTagContainer& RegularGameplayTags)
	{
		SelfType Result;
		Result.GameplayTagContainerValue = RegularGameplayTags;
		Result.Super::EnsureValidRootTag();
		return Result;
	}

	/** Create a container from filtered matching tags. */
	static SelfType CreateFiltered(const FGameplayTagContainer& RegularGameplayTags)
	{
		return CreateChecked(Super::FilterRootTag(RegularGameplayTags));
	}

private:
	FGameplayTagContainer& GetRef_Impl() { return GameplayTagContainerValue; }
	const FGameplayTagContainer& GetRef_Impl() const { return GameplayTagContainerValue; }

private:
	FGameplayTagContainer GameplayTagContainerValue;
};

//---------------------------------------------------------------------------------------------------------------------