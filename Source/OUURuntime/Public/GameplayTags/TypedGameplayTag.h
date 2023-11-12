// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "Engine/DeveloperSettings.h"
#include "GameplayTags/LiteralGameplayTag.h"
#include "GameplayTags/TypedGameplayTagSettings.h"
#include "Misc/CoreDelegates.h"

#include "TypedGameplayTag.generated.h"

//---------------------------------------------------------------------------------------------------------------------

namespace OUU::Runtime::Private
{
	template <typename... T>
	struct TGetAllTypedTagRootTags_Recursive;

	template <typename T, typename... Ts>
	struct TGetAllTypedTagRootTags_Recursive<T, Ts...>
	{
		static void Impl(FGameplayTagContainer& OutRootTags)
		{
			FGameplayTag RootTag = T::Get();
			OutRootTags.AddTag(RootTag);
			return TGetAllTypedTagRootTags_Recursive<Ts...>::Impl(OutRootTags);
		}
	};

	template <>
	struct TGetAllTypedTagRootTags_Recursive<>
	{
		static void Impl(FGameplayTagContainer& OutRootTags)
		{
			// do nothing
		}
	};

#if WITH_EDITOR
	OUURUNTIME_API FString MakeFilterString(const FGameplayTagContainer& GameplayTags);
#endif

} // namespace OUU::Runtime::Private

// Forward declare the derived container types...

// 1) The reference type points to an external gameplay tag container and should be preferred.
template <typename BlueprintTagType>
struct TTypedGameplayTagContainerReference;

// 2) The value type contains the gameplay tag container as member and can e.g. be used for return values.
template <typename BlueprintTagType>
struct TTypedGameplayTagContainerValue;

/**
 * Base class for blueprint/property exposed typesafe tags.
 * Typesafe in the sense that you can only assign child tags of the root tags.
 * Supports assignment and comparison with matching literal gameplay tags.
 *
 * Inspired by TTypedTagStaticImpl / FUITag
 *
 * Usage:
 *    - Subclass this and FGameplayTag
 *    - Assign Categories meta specifier to matching tag
 *    - use DEFINE_TYPED_GAMEPLAY_TAG macro in body.
 * See FOUUSampleTag.
 */
template <typename InBlueprintTagType, typename... InRootLiteralTagTypes>
struct TTypedGameplayTag
{
public:
	using BlueprintTagType = InBlueprintTagType;
	// using RootLiteralTagType = InRootLiteralTagType;
	using TypedGameplayTagType = TTypedGameplayTag<BlueprintTagType, InRootLiteralTagTypes...>;

	using ValueContainerType = TTypedGameplayTagContainerValue<BlueprintTagType>;
	using ReferenceContainerType = TTypedGameplayTagContainerReference<BlueprintTagType>;

	/**
	 * Get a list of the tags that are considered valid tag roots for this tag type.
	 */
	static ValueContainerType GetNativeTagRootTags()
	{
		FGameplayTagContainer Result;
		OUU::Runtime::Private::TGetAllTypedTagRootTags_Recursive<InRootLiteralTagTypes...>::Impl(OUT Result);
		return ValueContainerType::CreateUnchecked(Result);
	}

	static ValueContainerType GetAllRootTags()
	{
		FGameplayTagContainer Result;
		// Native tags
		OUU::Runtime::Private::TGetAllTypedTagRootTags_Recursive<InRootLiteralTagTypes...>::Impl(OUT Result);
		// Plus additional tags from settings
		UTypedGameplayTagSettings::GetAdditionalRootTags(OUT Result, BlueprintTagType::StaticStruct());
		return ValueContainerType::CreateUnchecked(Result);
	}

	static ValueContainerType GetAllLeafTags()
	{
		FGameplayTagContainer Result;
		UTypedGameplayTagSettings::GetAllLeafTags(OUT Result, BlueprintTagType::StaticStruct());
		return ValueContainerType::CreateUnchecked(Result);
	}

	template <typename T, typename U, typename V>
	static constexpr bool AssertLiteralGameplayTag(const TLiteralGameplayTag<T, U, V>& LiteralGameplayTag)
	{
		using ParamTagType = TLiteralGameplayTag<T, U, V>;

		static_assert(
			TIsDerivedFrom<BlueprintTagType, FGameplayTag>::Value,
			"BlueprintStructType must be derived from FGameplayTag");

		static_assert(
			TOr<TIsChildTagOf<ParamTagType, InRootLiteralTagTypes>...>::Value,
			"Can only assign from a literal gameplay tag that is nested under any of the InRootLiteralTagTypes");

		return true;
	}

	static BlueprintTagType TryConvert(FGameplayTag VanillaTag, bool bChecked)
	{
		ValueContainerType RootTags = GetAllRootTags();
		for (auto RootTag : RootTags)
		{
			if (VanillaTag.MatchesTag(RootTag))
			{
				return BlueprintTagType(VanillaTag);
			}
		}
		if (VanillaTag.IsValid() && bChecked)
		{
			checkf(
				VanillaTag == FGameplayTag::EmptyTag,
				TEXT("Tag %s is not part of the list of valid root tags %s and also not a completely empty tag"),
				*VanillaTag.ToString(),
				*RootTags.ToString());
		}
		return FGameplayTag::EmptyTag;
	}
};

/** Base class for typed gameplay tags */
USTRUCT()
struct OUURUNTIME_API FTypedGameplayTag_Base : public FGameplayTag
{
	GENERATED_BODY()
public:
	template <typename, typename...>
	friend struct TTypedGameplayTag;

	template <typename, typename>
	friend struct TTypedGameplayTagContainer_Base;

#if WITH_EDITOR
	static void RegisterAllDerivedPropertyTypeLayouts();
	static void UnregisterAllDerivedPropertyTypeLayouts();
#endif

public:
	/* Get a short display name for the given tag, relative to its root tags. */
	static FString ToShortDisplayString(const FGameplayTag& Tag, const FGameplayTagContainer& RootTags);
};

/**
 * Use in derived types of TLiteralGameplayTagInstance to inherit the literal gameplay tag functionality.
 * Does not support usage together with custom constructors!
 */
#define IMPLEMENT_TYPED_GAMEPLAY_TAG(TagType, ...) PRIVATE_TYPED_GAMEPLAY_TAG_IMPL(TagType, ##__VA_ARGS__)

//---------------------------------------------------------------------------------------------------------------------

#define PRIVATE_TYPED_GAMEPLAY_TAG_IMPL(TagType, ...)                                                                  \
public:                                                                                                                \
	using TypedTagImplType = TTypedGameplayTag<TagType, ##__VA_ARGS__>;                                                \
	template <typename, typename, typename>                                                                            \
	friend struct ::TLiteralGameplayTag;                                                                               \
	TagType() = default;                                                                                               \
                                                                                                                       \
	template <typename T, typename U, typename V>                                                                      \
	TagType(const TLiteralGameplayTag<T, U, V>& LiteralGameplayTag)                                                    \
	{                                                                                                                  \
		TypedTagImplType::AssertLiteralGameplayTag(LiteralGameplayTag);                                                \
		*this = LiteralGameplayTag.GetTag();                                                                           \
	}                                                                                                                  \
	TagType& operator=(const TagType& Other)                                                                           \
	{                                                                                                                  \
		static_cast<FGameplayTag&>(*this) = static_cast<const FGameplayTag&>(Other);                                   \
		return *this;                                                                                                  \
	}                                                                                                                  \
	FString ToShortDisplayString() const                                                                               \
	{                                                                                                                  \
		return FTypedGameplayTag_Base::ToShortDisplayString(*this, GetAllRootTags().Get());                            \
	}                                                                                                                  \
	static TagType TryConvert(FGameplayTag FromTag) { return TypedTagImplType::TryConvert(FromTag, false); }           \
	static TagType ConvertChecked(FGameplayTag FromTag) { return TypedTagImplType::TryConvert(FromTag, true); }        \
	static TypedTagImplType::ValueContainerType GetAllRootTags() { return TypedTagImplType::GetAllRootTags(); }        \
	static TypedTagImplType::ValueContainerType GetAllLeafTags() { return TypedTagImplType::GetAllLeafTags(); }        \
                                                                                                                       \
protected:                                                                                                             \
	FName GetStructName() const { return *TagType::StaticStruct()->GetName().Mid(1); }                                 \
                                                                                                                       \
private:                                                                                                               \
	TagType(FGameplayTag Tag) { TagName = Tag.GetTagName(); }                                                          \
                                                                                                                       \
	template <typename, typename...>                                                                                   \
	friend struct ::TTypedGameplayTag;                                                                                 \
                                                                                                                       \
	friend struct ::TTypedGameplayTagContainerIterator<TagType>;                                                       \
                                                                                                                       \
private:                                                                                                               \
	/* This helper is used for settings registration */                                                                \
	struct FAutoRegistrationHelper                                                                                     \
	{                                                                                                                  \
		FAutoRegistrationHelper();                                                                                     \
		~FAutoRegistrationHelper();                                                                                    \
	};                                                                                                                 \
	static FAutoRegistrationHelper Instance;                                                                           \
                                                                                                                       \
	/* sorry :'( */                                                                                                    \
	}                                                                                                                  \
	;                                                                                                                  \
	static_assert(                                                                                                     \
		TIsDerivedFrom<TagType, FTypedGameplayTag_Base>::Value,                                                        \
		"Typed Gameplay Tags must be derived from FTypedGameplayTag_Base");                                            \
                                                                                                                       \
	using TagType##s_Value = TTypedGameplayTagContainerValue<TagType>;                                                 \
	using TagType##s_Ref = TTypedGameplayTagContainerReference<TagType>;                                               \
	template <>                                                                                                        \
	struct TStructOpsTypeTraits<TagType> : public TStructOpsTypeTraitsBase2<TagType>                                   \
	{                                                                                                                  \
		enum                                                                                                           \
		{                                                                                                              \
			WithNetSerializer = true,                                                                                  \
			WithNetSharedSerialization = true,                                                                         \
			WithPostSerialize = true,                                                                                  \
			WithStructuredSerializeFromMismatchedTag = true,                                                           \
			WithImportTextItem = true,                                                                                 \
		};

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

#define DEFINE_TYPED_GAMEPLAY_TAG(TagType)                                                                             \
	TagType::FAutoRegistrationHelper::FAutoRegistrationHelper()                                                        \
	{                                                                                                                  \
		FCoreDelegates::OnAllModuleLoadingPhasesComplete.AddLambda([]() {                                              \
			UTypedGameplayTagSettings::AddNativeRootTags(                                                              \
				TypedTagImplType::GetNativeTagRootTags().Get(),                                                        \
				TagType::StaticStruct());                                                                              \
		});                                                                                                            \
	}                                                                                                                  \
	TagType::FAutoRegistrationHelper::~FAutoRegistrationHelper() {}                                                    \
	TagType::FAutoRegistrationHelper TagType::Instance;

//---------------------------------------------------------------------------------------------------------------------

template <typename BlueprintTagType>
struct TTypedGameplayTagContainerIterator
{
public:
	using WrappedIteratorType = TArray<FGameplayTag>::TConstIterator;
	using SizeType = decltype(DeclVal<WrappedIteratorType>().GetIndex());

	TTypedGameplayTagContainerIterator(WrappedIteratorType InWrappedIterator) : WrappedIterator(InWrappedIterator) {}

	TTypedGameplayTagContainerIterator& operator++()
	{
		++WrappedIterator;
		return *this;
	}
	TTypedGameplayTagContainerIterator& operator++(int)
	{
		TTypedGameplayTagContainerIterator Tmp(*this);
		++WrappedIterator;
		return Tmp;
	}

	TTypedGameplayTagContainerIterator& operator--()
	{
		--WrappedIterator;
		return *this;
	}
	TTypedGameplayTagContainerIterator operator--(int)
	{
		TTypedGameplayTagContainerIterator Tmp(*this);
		--WrappedIterator;
		return Tmp;
	}

	TTypedGameplayTagContainerIterator& operator+=(SizeType Offset)
	{
		WrappedIterator += Offset;
		return *this;
	}

	TTypedGameplayTagContainerIterator operator+(SizeType Offset) const
	{
		TTypedGameplayTagContainerIterator Tmp(*this);
		return Tmp += Offset;
	}

	TTypedGameplayTagContainerIterator& operator-=(SizeType Offset) { return *this += -Offset; }

	TTypedGameplayTagContainerIterator operator-(SizeType Offset) const
	{
		TTypedGameplayTagContainerIterator Tmp(*this);
		return Tmp -= Offset;
	}

	FORCEINLINE BlueprintTagType operator*() const { return BlueprintTagType(*WrappedIterator); }

	FORCEINLINE explicit operator bool() const { return !!WrappedIterator; }

	FORCEINLINE friend bool operator==(
		const TTypedGameplayTagContainerIterator& Lhs,
		const TTypedGameplayTagContainerIterator& Rhs)
	{
		return Lhs.WrappedIterator == Rhs.WrappedIterator;
	}
	FORCEINLINE friend bool operator!=(
		const TTypedGameplayTagContainerIterator& Lhs,
		const TTypedGameplayTagContainerIterator& Rhs)
	{
		return Lhs.WrappedIterator != Rhs.WrappedIterator;
	}

private:
	TArray<FGameplayTag>::TConstIterator WrappedIterator;
};

// #TODO-jreich Move to private namespace

/**
 * Template wrapper for gameplay tag container that only allows assignment of matching gameplay tags and tag containers.
 */
template <typename InBlueprintTagType, typename DerivedImplementationType>
struct TTypedGameplayTagContainer_Base
{
public:
	using BlueprintTagType = InBlueprintTagType;
	using TypedTagImplType = typename BlueprintTagType::TypedTagImplType;

	using ValueContainerType = TTypedGameplayTagContainerValue<BlueprintTagType>;
	using ReferenceContainerType = TTypedGameplayTagContainerReference<BlueprintTagType>;

	using IteratorType = TTypedGameplayTagContainerIterator<BlueprintTagType>;

public:
	FORCEINLINE const FGameplayTagContainer& Get() const { return GetRef(); }

	static FGameplayTagContainer FilterRootTag(const FGameplayTagContainer& RegularGameplayTags)
	{
		return RegularGameplayTags.Filter(TypedTagImplType::GetAllRootTags().Get());
	}

	// Call this in derived constructors / conversion functions.
	void EnsureValidRootTag() const
	{
#if DO_CHECK
		const FGameplayTagContainer RootTags = TypedTagImplType::GetAllRootTags().Get();
		for (auto& Tag : GetRef())
		{
			ensureMsgf(
				Tag.MatchesAny(RootTags),
				TEXT("Typed gameplay tag container contains tag '%s' that does not match any of the required root tags "
					 "%s"),
				*Tag.ToString(),
				*RootTags.ToString());
		}
#endif
	}

	// Support for ranged for loops
	auto begin() const { return IteratorType(GetRef().CreateConstIterator()); }
	auto end() const
	{
		auto Result = GetRef().CreateConstIterator();
		Result.SetToEnd();
		return IteratorType(Result);
	}

protected:
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
	void AppendTags(const ReferenceContainerType& Other) { return GetRef().AppendTags(Other.Get()); }
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
	BlueprintTagType GetByIndex(int32 Index) const
	{
		return BlueprintTagType::ConvertChecked(GetRef().GetByIndex(Index));
	}
	BlueprintTagType First() const { return BlueprintTagType::ConvertChecked(GetRef().First()); }
	BlueprintTagType Last() const { return BlueprintTagType::ConvertChecked(GetRef().Last()); }

	// Can't be sure that these are still underneath the root tag, so just don't allow it
	// void FillParentTags() { return GetRef().FillParentTags(); }

	bool operator==(ReferenceContainerType const& Other) const { return GetRef() == Other.GetRef(); }
	bool operator!=(ReferenceContainerType const& Other) const { return GetRef() != Other.GetRef(); }
#pragma endregion GameplayTagContainerAPI
	// -- end of functions mirroring the FGameplayTagContainer API
};

//---------------------------------------------------------------------------------------------------------------------

template <typename InBlueprintTagType>
struct TTypedGameplayTagContainerReference :
	public TTypedGameplayTagContainer_Base<InBlueprintTagType, TTypedGameplayTagContainerReference<InBlueprintTagType>>
{
	template <typename, typename>
	friend struct TTypedGameplayTagContainer_Base;

	using BlueprintTagType = InBlueprintTagType;
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
	public TTypedGameplayTagContainer_Base<InBlueprintTagType, TTypedGameplayTagContainerValue<InBlueprintTagType>>
{
	template <typename, typename>
	friend struct TTypedGameplayTagContainer_Base;

	template <typename, typename...>
	friend struct TTypedGameplayTag;

	using BlueprintTagType = InBlueprintTagType;
	using SelfType = TTypedGameplayTagContainerValue<BlueprintTagType>;
	using Super = TTypedGameplayTagContainer_Base<BlueprintTagType, SelfType>;

public:
	TTypedGameplayTagContainerValue() = default;
	// explicit conversion from ref type (so we don't create accidental copies)
	explicit TTypedGameplayTagContainerValue(const TTypedGameplayTagContainerReference<BlueprintTagType>& Ref) :
		GameplayTagContainerValue(Ref.Get())
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
	/**
	 * Create a contianer from another container that is assumed to be pre-filtered (faster) WITHOUT checking.
	 * This is less safe, but sometimes necessary to avoid infinite recursion.
	 */
	static SelfType CreateUnchecked(const FGameplayTagContainer& RegularGameplayTags)
	{
		SelfType Result;
		Result.GameplayTagContainerValue = RegularGameplayTags;
		return Result;
	}

	FGameplayTagContainer& GetRef_Impl() { return GameplayTagContainerValue; }
	const FGameplayTagContainer& GetRef_Impl() const { return GameplayTagContainerValue; }

private:
	FGameplayTagContainer GameplayTagContainerValue;
};

//---------------------------------------------------------------------------------------------------------------------
