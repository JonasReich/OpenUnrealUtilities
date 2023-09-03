// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "GameplayTagContainer.h"
#include "GameplayTags/TypedGameplayTag.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "TypedGameplayTagContainer.generated.h"

/**
 * This struct is a gameplay tag container that behaves like TTypedGameplayTagContainerValue, but a little bit less safe
 * (runtime type checks instead of C++ compile time checks).
 * It's blueprint exposed and can be used for Blueprint properties as well. The type for the gameplay tag container can
 * only be changed on the class level where the property is created, but not on child classes.
 */
USTRUCT(BlueprintType)
struct OUURUNTIME_API FTypedGameplayTagContainer
{
	GENERATED_BODY()

	friend class FTypedGameplayTagContainer_PropertyTypeCustomiation;

public:
	FTypedGameplayTagContainer() = default;

private:
	FTypedGameplayTagContainer(UStruct& InTypedTagStruct, const FGameplayTagContainer& InTags);

public:
	template <typename TypedTagType>
	FTypedGameplayTagContainer(const TArray<FGameplayTag>& InTags) :
		FTypedGameplayTagContainer(
			*TypedTagType::StaticStruct(),
			TTypedGameplayTagContainerValue<TypedTagType>::CreateChecked(FGameplayTagContainer::FromArray(InTags)))
	{
		static_assert(
			TIsDerivedFrom<TypedTagType, FTypedGameplayTag_Base>::Value,
			"TypedTagType must be derived from FTypedGameplayTag_Base");
	}

	template <typename TypedTagType>
	FTypedGameplayTagContainer(std::initializer_list<FGameplayTag> InTags) :
		FTypedGameplayTagContainer(*TypedTagType::StaticStruct(), TArray<FGameplayTag>(InTags))
	{
		static_assert(
			TIsDerivedFrom<TypedTagType, FTypedGameplayTag_Base>::Value,
			"TypedTagType must be derived from FTypedGameplayTag_Base");
	}

	const FGameplayTagContainer& GetTags() const { return Tags; }
	const FName& GetTypedTagName() const { return TypedTagName; }

	// -----
	// Edit functions. Identical to FGameplayTagContainer except these apply the typed tag filter afterwards.
	// -----
	void SetTags(const FGameplayTagContainer& InTags);
	void AddTag(const FGameplayTag& TagToAdd);
	void AppendTags(FGameplayTagContainer const& TagsToAppend);
	bool RemoveTag(const FGameplayTag& TagToRemove, bool bDeferParentTags = false);
	void RemoveTags(const FGameplayTagContainer& TagsToRemove);
	void Reset(int32 Slack = 0);

private:
	// Underlying tag container
	// The "TypedGameplayTagContainer" filter tells our editor customization to look for the TypedTagName property on
	// the owning struct property for tag filter info.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess, Categories = "TypedGameplayTagContainer"))
	FGameplayTagContainer Tags;

	// Name of a typed gameplay tag type, e.g. "OUUSampleBarTag" for the FOUUSampleBarTag type.
	// This is the same string used inside the curly-brace filter string you can use for FGameplayTagContainers in C++
	// code (e.g. "TypedTag{OUUSampleBarTag}").
	// This is only editable at "construct time" on the class a container property is first introduced i.e. either in
	// constructor (C++) or class defaults (Blueprint). Once it's actually used to store tags, the typed tag name should
	// ideally not be edited anymore at all.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess))
	FName TypedTagName;

	// -- Implicit info from typed tag name

	// Tag container with filter tags when calling edit functions
	FGameplayTagContainer CachedFilterTags;

	void PopulateFilterTags();
};

UCLASS(BlueprintType)
class UTypedGameplayTagContainerLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable)
	static void SetTypedContainerTags(
		UPARAM(ref) FTypedGameplayTagContainer& Container,
		const FGameplayTagContainer& Tags);

	UFUNCTION(BlueprintCallable)
	static void AddTagToTypedContainer(UPARAM(ref) FTypedGameplayTagContainer& Container, const FGameplayTag& TagToAdd);

	UFUNCTION(BlueprintCallable)
	static void AppendTagsToTypedContainer(
		UPARAM(ref) FTypedGameplayTagContainer& Container,
		FGameplayTagContainer const& TagsToAppend);

	UFUNCTION(BlueprintCallable)
	static bool RemoveTagFromTypedContainer(
		UPARAM(ref) FTypedGameplayTagContainer& Container,
		const FGameplayTag& TagToRemove);

	UFUNCTION(BlueprintCallable)
	static void RemoveTagsFromTypedContainer(
		UPARAM(ref) FTypedGameplayTagContainer& Container,
		const FGameplayTagContainer& TagsToRemove);

	UFUNCTION(BlueprintCallable)
	static void ResetTypedContainer(UPARAM(ref) FTypedGameplayTagContainer& Container);
};
