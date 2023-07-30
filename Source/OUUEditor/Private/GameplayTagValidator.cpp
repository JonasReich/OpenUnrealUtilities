// Copyright (c) 2023 Jonas Reich & Contributors

#include "GameplayTagValidator.h"

#include "Editor.h"
#include "GameplayTagsManager.h"
#include "GameplayTagsModule.h"
#include "LogOpenUnrealUtilities.h"
#include "Logging/MessageLogMacros.h"
#include "Misc/DataValidation.h"

FAutoConsoleCommand GTagsValidateCommand{
	TEXT("ouu.Tags.Validate"),
	TEXT("Run tags validation on all registered gameplay tags"),
	FConsoleCommandDelegate::CreateLambda([]() { UGameplayTagValidatorSubsystem::Get().ValidateGameplayTagTree(); })};

void UGameplayTagValidationSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (bValidateTagsAfterSettingsChange)
	{
		UGameplayTagValidatorSubsystem::Get().ValidateGameplayTagTree();
	}
}

UGameplayTagValidatorSubsystem& UGameplayTagValidatorSubsystem::Get()
{
	return *GEditor->GetEditorSubsystem<UGameplayTagValidatorSubsystem>();
}

void UGameplayTagValidatorSubsystem::ValidateGameplayTagTree()
{
	auto Validators = GetAllValidators();

	if (Validators.Num() == 0)
	{
		UE_LOG(LogOpenUnrealUtilities, Error, TEXT("No non-abstract gameplay tag validator classes found."));
		return;
	}

	auto& TagsManager = UGameplayTagsManager::Get();
	TArray<TSharedPtr<FGameplayTagNode>> RootNodes;
	TagsManager.GetFilteredGameplayRootTags(FString(), OUT RootNodes);

	FDataValidationContext ValidationContext;

	for (auto& RootNode : RootNodes)
	{
		auto& RootTag = RootNode->GetCompleteTag();
		// The root tags do not have a parent
		auto& ParentTag = FGameplayTag::EmptyTag;
		ValidateTagNode(RootTag, ParentTag, RootNode, Validators, IN OUT ValidationContext);
	}

	TArray<FText> Warnings, Errors;
	ValidationContext.SplitIssues(OUT Warnings, OUT Errors);

#define MESSAGELOG_CAT AssetCheck
	auto MessageLogName = GetMessageLogName(EMessageLogName::MESSAGELOG_CAT);

	UMessageLogBlueprintLibrary::NewMessageLogPage(MessageLogName, INVTEXT("Gameplay Tag Validation"));

	for (auto& Error : Errors)
	{
		UE_MESSAGELOG(MESSAGELOG_CAT, Error, Error);
	}
	for (auto& Warning : Warnings)
	{
		UE_MESSAGELOG(MESSAGELOG_CAT, Warning, Warning);
	}

	if (Errors.Num() > 0)
	{
		UMessageLogBlueprintLibrary::NotifyMessageLog(
			MessageLogName,
			FText::Format(INVTEXT("{0} GameplayTag validation errors"), Errors.Num()),
			EMessageLogSeverity::Info);
	}
	else if (Warnings.Num() > 0)
	{
		UMessageLogBlueprintLibrary::NotifyMessageLog(
			MessageLogName,
			FText::Format(INVTEXT("{0} GameplayTag validation warnings"), Warnings.Num()),
			EMessageLogSeverity::Info);
	}
	else
	{
		UE_MESSAGELOG(MESSAGELOG_CAT, Info, "No GameplayTag validation issues found.");
	}
#undef MESSAGELOG_CAT
}

void UGameplayTagValidatorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	IGameplayTagsModule::Get()
		.OnGameplayTagTreeChanged.AddUObject(this, &UGameplayTagValidatorSubsystem::ValidateGameplayTagTree);
}

void UGameplayTagValidatorSubsystem::Deinitialize()
{
	if (IGameplayTagsModule::IsAvailable())
	{
		IGameplayTagsModule::Get().OnGameplayTagTreeChanged.RemoveAll(this);
	}
	Super::Deinitialize();
}

TArray<UGameplayTagValidator_Base*> UGameplayTagValidatorSubsystem::GetAllValidators()
{
	TArray<UClass*> ValidatorClasses;
	GetDerivedClasses(UGameplayTagValidator_Base::StaticClass(), OUT ValidatorClasses);

	TArray<UGameplayTagValidator_Base*> Validators;
	for (auto* ValidatorClass : ValidatorClasses)
	{
		if (ValidatorClass->HasAnyClassFlags(CLASS_Abstract | CLASS_NewerVersionExists))
			continue;

		if (auto* CastedCDO = Cast<UGameplayTagValidator_Base>(ValidatorClass->GetDefaultObject()))
		{
			Validators.Add(CastedCDO);
			CastedCDO->InitializeValidator();
		}
	}
	return Validators;
}

void UGameplayTagValidatorSubsystem::ValidateTagNode(
	const FGameplayTag& RootTag,
	const FGameplayTag& ImmediateParentTag,
	const TSharedPtr<FGameplayTagNode>& TagNode,
	const TArray<UGameplayTagValidator_Base*>& Validators,
	FDataValidationContext& InOutValidationContext)
{
	auto& TagsManager = UGameplayTagsManager::Get();

	auto SelfTag = TagNode->GetCompleteTag();
	TArray<FName> Names;
	TagsManager.SplitGameplayTagFName(SelfTag, OUT Names);

	for (auto* Validator : Validators)
	{
		Validator->ValidateTag(RootTag, ImmediateParentTag, SelfTag, IN Names, IN OUT InOutValidationContext);
	}

	auto ChildNodes = TagNode->GetChildTagNodes();
	for (auto& ChildNode : ChildNodes)
	{
		auto ChildTag = ChildNode->GetCompleteTag();
		ValidateTagNode(RootTag, SelfTag, ChildNode, Validators, IN OUT InOutValidationContext);
	}
}

void UGameplayTagValidatorSubsystem::HandleGameplayTagTreeChanged()
{
	auto& Settings = *GetDefault<UGameplayTagValidationSettings>();
	if (Settings.bValidateTagsAfterTagTreeChange)
	{
		ValidateGameplayTagTree();
	}
}

void UOUUGameplayTagValidator::InitializeValidator()
{
	AllNativeTags.Reset();

	auto& TagsManager = UGameplayTagsManager::Get();
	TArray<TSharedPtr<FGameplayTagNode>> NativeTagNodes;
	TagsManager.GetAllTagsFromSource(FGameplayTagSource::GetNativeName(), OUT NativeTagNodes);
	for (auto NativeTagNode : NativeTagNodes)
	{
		AllNativeTags.AddTag(NativeTagNode->GetCompleteTag());
	}
}

void UOUUGameplayTagValidator::ValidateTag(
	const FGameplayTag& RootTag,
	const FGameplayTag& ImmediateParentTag,
	const FGameplayTag& Tag,
	const TArray<FName>& TagComponents,
	FDataValidationContext& InOutValidationContext)
{
	const UGameplayTagValidationSettings& Settings = *GetDefault<UGameplayTagValidationSettings>();

	auto AddIssue = [&](const FText& IssueText) {
		for (auto& WarnOnlyTag : Settings.WarnOnlyGameplayTags)
		{
			if (Tag.MatchesTag(WarnOnlyTag))
			{
				InOutValidationContext.AddWarning(IssueText);
				return;
			}
		}
		InOutValidationContext.AddError(IssueText);
	};

	const bool bTagIsNative = AllNativeTags.HasTagExact(Tag);
	const bool bTagIsRoot = RootTag == Tag;

	if ((Settings.bAllowContentRootTags == false) && bTagIsRoot && (bTagIsNative == false))
	{
		AddIssue(FText::Format(
			INVTEXT(
				"{0}: Content tags are not permitted as root tags. Please declare it as a Native tag in C++ code or "
				"enable {1}"),
			FText::FromString(Tag.ToString()),
			FText::FromString(PREPROCESSOR_TO_STRING(UGameplayTagValidationSettings::bAllowContentRootTags))));
		return;
	}

	auto ParentTagsAsContainer = Tag.GetGameplayTagParents();

	auto CurrentTagDepth = ParentTagsAsContainer.Num();
	if (CurrentTagDepth > Settings.MaxGlobalTagDepth)
	{
		// This tag depth rule should apply both for native and content tags, so we check it before.
		AddIssue(FText::Format(
			INVTEXT("{0} is too deep ({1}). MaxGlobalTagDepth is {2}"),
			FText::FromString(Tag.ToString()),
			FText::AsNumber(CurrentTagDepth),
			FText::AsNumber(Settings.MaxGlobalTagDepth)));
		return;
	}

	TArray<FGameplayTag> ParentTagsAsSortedArray;
	for (auto ParentTag : ParentTagsAsContainer)
	{
		ParentTagsAsSortedArray.Add(ParentTag);
	}

	// Sort by how deep the tags match. Earlier entries are closer to the tag. Later entries are closer to the root.
	ParentTagsAsSortedArray.Sort([&Tag](const FGameplayTag& A, const FGameplayTag& B) -> bool {
		return A.MatchesTagDepth(Tag) > B.MatchesTagDepth(Tag);
	});

	ensure(ParentTagsAsSortedArray[0] == Tag);
	ensure(ParentTagsAsSortedArray.Last() == RootTag);

	auto& TagsManager = UGameplayTagsManager::Get();

	bool bFoundAllowRule = false;
	if (bTagIsNative)
	{
		// #TODO checks for native tags
	}
	else
	{
		int32 NativeRelativeTagDepth = 0;
		auto FirstNativeTag = FGameplayTag::EmptyTag;
		for (auto& Parent : ParentTagsAsSortedArray)
		{
			if (AllNativeTags.HasTagExact(Parent))
			{
				// Parent is Native
				if (auto* SettingsEntry = Settings.TagOverrides.Find(Parent))
				{
					if (SettingsEntry->bCanHaveContentChildren == false)
					{
						AddIssue(FText::Format(
							INVTEXT("{0} is a child of tag {1} which explicitly forbids creating child content tags"),
							FText::FromString(Tag.ToString()),
							FText::FromString(Parent.ToString())));
					}
					else
					{
						bFoundAllowRule = true;

						if (SettingsEntry->AllowedChildDepth < NativeRelativeTagDepth)
						{
							AddIssue(FText::Format(
								INVTEXT("{0} is a {1} level deep child of tag {2}, but that parent tags only allows "
										"content "
										"children of up to {3} levels"),
								FText::FromString(Tag.ToString()),
								FText::AsNumber(NativeRelativeTagDepth),
								FText::FromString(Parent.ToString()),
								FText::AsNumber(SettingsEntry->AllowedChildDepth)));
						}
					}
				}

				FirstNativeTag = Parent;

				// Break at the first native tag.
				// Native / Content can't be mixed.
				// Only exception: We could encounter some implcit tags from native tags that are also declared
				// explicitly in content. We ignore those tags for this check.
				break;
			}

			NativeRelativeTagDepth += 1;
		}

		if (FirstNativeTag == FGameplayTag::EmptyTag)
		{
			// Root tag handling is already covered above.
		}
		else if (bFoundAllowRule == false)
		{
			if (Settings.bDefaultAllowContentTagChildren)
			{
				// ok: content tags are allowed by default
			}
			else
			{
				AddIssue(FText::Format(
					INVTEXT("{0} is a content tag created below native tag {1}, but there was no rule explicitly "
							"allowing content tags below that native tag. You can enable {2} to allow content tags "
							"unless explicitly prohibited."),
					FText::FromString(Tag.ToString()),
					FText::FromString(FirstNativeTag.ToString()),
					FText::FromString(
						PREPROCESSOR_TO_STRING(UGameplayTagValidationSettings::bDefaultAllowContentTagChildren))));
			}
		}
	}
}
