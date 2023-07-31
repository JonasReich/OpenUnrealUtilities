// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "EditorSubsystem.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"

#include "GameplayTagValidator.generated.h"

USTRUCT()
struct FGameplayTagValidationSettingsEntry
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere)
	bool bCanHaveContentChildren = true;

	UPROPERTY(EditAnywhere)
	int32 AllowedChildDepth = 1;
};

/**
 * Configure how gameplay tags across the project are validated.
 * Projects can implement custom validators in C++ code in addition to this (see UGameplayTagValidator_Base).
 */
UCLASS(Config = GameplayTags, DefaultConfig)
class UGameplayTagValidationSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	// Maximum nesting depth of tags including the root tags.
	// e.g. 'Foo' has a nesting level of 1, 'Foo.Bar' has 2, 'Foo.Bar.Baz' has 3, etc.
	UPROPERTY(Config, EditAnywhere, meta = (UIMin=1, UIMax=20))
	int32 MaxGlobalTagDepth = 10;

	// Default depth allowed for native tags that are marked as "allow child tags" from C++ code.
	// You can always create tag overrides that supercede this setting for individual tags.
	UPROPERTY(Config, EditAnywhere)
	int32 NativeTagAllowedChildDepth = 3;

	UPROPERTY(Config, EditAnywhere)
	bool bAllowContentRootTags = false;

	// If true, allow content tags as children anywhere they are not explicitly prohibited via TagOverrides.
	UPROPERTY(Config, EditAnywhere)
	bool bDefaultAllowContentTagChildren = false;

	// If true, run gameplay tag validation after every change of the gameplay tags tree.
	// This means both after editor start and every edit of the tags list.
	UPROPERTY(Config, EditAnywhere)
	bool bValidateTagsAfterTagTreeChange = true;

	UPROPERTY(Config, EditAnywhere)
	bool bValidateTagsDuringCook = true;

	// If true, run gameplay tag validation after every change of settings in this settings class.
	UPROPERTY(Config, EditAnywhere)
	bool bValidateTagsAfterSettingsChange = true;

	// Issues underneath these gameplay tags will always only cause warnings instead of errors.
	// Only affects issues from UOUUGameplayTagValidator. Other validator classes may ignore this setting.
	UPROPERTY(Config, EditAnywhere)
	FGameplayTagContainer WarnOnlyGameplayTags;

	void RefreshNativeTagOverrides();
	const FGameplayTagValidationSettingsEntry* FindTagOverride(FGameplayTag Tag) const;

public:
	// - UDeveloperSettings
	FName GetCategoryName() const override;
	FText GetSectionText() const override;
	// --

	// - UObject
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	// --

private:
	UPROPERTY(Config, EditAnywhere, meta = (ForceInlineRow))
	TMap<FGameplayTag, FGameplayTagValidationSettingsEntry> TagOverrides;

	// Settings declared in code from literal gameplay tags
	UPROPERTY(VisibleAnywhere, meta = (ForceInlineRow))
	TMap<FGameplayTag, FGameplayTagValidationSettingsEntry> NativeTagOverrides;
};

UCLASS(Abstract)
class OUUEDITOR_API UGameplayTagValidator_Base : public UObject
{
	GENERATED_BODY()
public:
	virtual void InitializeValidator() PURE_VIRTUAL(UGameplayTagValidator_Base::InitializeValidator);
	virtual void ValidateTag(
		const FGameplayTag& RootTag,
		const FGameplayTag& ParentTag,
		const FGameplayTag& Tag,
		const TArray<FName>& TagComponents,
		FDataValidationContext& InOutValidationContext) PURE_VIRTUAL(UGameplayTagValidator_Base::ValidateTag);
};

UCLASS(BlueprintType)
class OUUEDITOR_API UGameplayTagValidatorSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()
public:
	static UGameplayTagValidatorSubsystem& Get();

	UFUNCTION(BlueprintCallable)
	void ValidateGameplayTagTree();

	// - UEngineSubsystem
	void Initialize(FSubsystemCollectionBase& Collection) override;
	void Deinitialize() override;
	// --

private:
	// Returns a list of all validators and initializes them.
	static TArray<UGameplayTagValidator_Base*> GetAllValidators();

	void ValidateTagNode(
		const FGameplayTag& RootTag,
		const FGameplayTag& ImmediateParentTag,
		const TSharedPtr<FGameplayTagNode>& TagNode,
		const TArray<UGameplayTagValidator_Base*>& Validators,
		FDataValidationContext& InOutValidationContext);

	UFUNCTION()
	void HandleGameplayTagTreeChanged();
};

// Validates some rules imposed by Open Unreal Utilities.
// All of the rules are configured in UGameplayTagValidationSettings.
UCLASS()
class UOUUGameplayTagValidator : public UGameplayTagValidator_Base
{
	GENERATED_BODY()
public:
	// - UGameplayTagValidator_Base
	void InitializeValidator() override;
	void ValidateTag(
		const FGameplayTag& RootTag,
		const FGameplayTag& ImmediateParentTag,
		const FGameplayTag& Tag,
		const TArray<FName>& TagComponents,
		FDataValidationContext& InOutValidationContext) override;
	// --
private:
	FGameplayTagContainer AllNativeTags;
};
