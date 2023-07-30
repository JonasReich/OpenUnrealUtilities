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

UCLASS(Config = Editor, DefaultConfig)
class UGameplayTagValidationSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	UPROPERTY(Config, EditAnywhere, meta = (ForceInlineRow))
	TMap<FGameplayTag, FGameplayTagValidationSettingsEntry> TagOverrides;

	UPROPERTY(Config, EditAnywhere)
	int32 MaxGlobalTagDepth = 10;

	UPROPERTY(Config, EditAnywhere)
	bool bAllowContentRootTags = false;

	// If true, allow content tags as children anywhere they are not explicitly prohibited via TagOverrides.
	UPROPERTY(Config, EditAnywhere)
	bool bDefaultAllowContentTagChildren = false;

	// If true, run gameplay tag validation after every change of the gameplay tags tree.
	// This means both after editor start and every edit of the tags list.
	UPROPERTY(Config, EditAnywhere)
	bool bValidateTagsAfterTagTreeChange = true;

	// If true, run gameplay tag validation after every change of settings in this settings class.
	UPROPERTY(Config, EditAnywhere)
	bool bValidateTagsAfterSettingsChange = true;

	// Issues underneath these gameplay tags will always only cause warnings instead of errors.
	// Only affects issues from UOUUGameplayTagValidator. Other validator classes may ignore this setting.
	UPROPERTY(Config, EditAnywhere)
	FGameplayTagContainer WarnOnlyGameplayTags;

public:
	// - UObject
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	// --
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
