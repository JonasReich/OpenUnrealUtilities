// Copyright (c) 2024 Jonas Reich & Contributors

#include "AssetRegistry/AssetRegistryModule.h"
#include "GameplayTagsManager.h"
#include "LogOpenUnrealUtilities.h"

namespace OUU::Editor::GameplayTagAudit
{
	void PrintGameplayTagAndDepsRecursive(
		const TSharedPtr<FGameplayTagNode>& TagNode,
		int32& InOutNumExplicitTags,
		int32& InOutNumImplicitTags,
		int32& InOutNumReferencers,
		int32& InOutNumReferencersInclusive,
		int32& InOutNumUnreferencedTags,
		TArray<int32>& InOutNumReferencersArray)
	{
		const auto Tag = TagNode->GetCompleteTag();

		const FAssetIdentifier TagAssetIdentifier(FGameplayTag::StaticStruct(), Tag.GetTagName());

		const auto& AssetRegistry = FAssetRegistryModule::GetRegistry();
		TArray<FAssetIdentifier> TagReferencers;
		AssetRegistry.GetReferencers(TagAssetIdentifier, OUT TagReferencers);

		FString TagComment;
		FName TagSource;
		bool bTagIsExplicit = false, bIsRestrictedTag = false, bAllowNonRestrictedChildren = false;
		UGameplayTagsManager::Get().GetTagEditorData(
			Tag.GetTagName(),
			OUT TagComment,
			OUT TagSource,
			OUT bTagIsExplicit,
			OUT bIsRestrictedTag,
			OUT bAllowNonRestrictedChildren);

		++(bTagIsExplicit ? InOutNumExplicitTags : InOutNumImplicitTags);
		InOutNumReferencers += TagReferencers.Num();

		const int32 NumReferencersExclusive = TagReferencers.Num();
		InOutNumReferencersArray.Add(NumReferencersExclusive);

		// Inclusive referencers for this tag include the exclusive tags plus all tags from children,
		// but not from parents that were passed in.
		int32 NumReferencersInclusiveThis = NumReferencersExclusive;
		for (const auto& ChildTagNode : TagNode->GetChildTagNodes())
		{
			PrintGameplayTagAndDepsRecursive(
				ChildTagNode,
				InOutNumExplicitTags,
				InOutNumImplicitTags,
				InOutNumReferencers,
				IN OUT NumReferencersInclusiveThis,
				InOutNumUnreferencedTags,
				InOutNumReferencersArray);
		}

		if (NumReferencersInclusiveThis == 0)
		{
			++InOutNumUnreferencedTags;
		}

		UE_LOG(
			LogOpenUnrealUtilities,
			Log,
			TEXT("%s, %s, %s, %i, %i"),
			*Tag.ToString(),
			*TagSource.ToString(),
			*LexToString(bTagIsExplicit),
			NumReferencersExclusive,
			NumReferencersInclusiveThis);

		// For the parent tag inclusive count: Accumulate all the inclusive tag counts
		InOutNumReferencersInclusive += NumReferencersInclusiveThis;
	}

	void PrintGameplayTagAudit()
	{
		UE_LOG(LogOpenUnrealUtilities, Log, TEXT("Gameplay Tag Audit START"));
		UE_LOG(LogOpenUnrealUtilities, Log, TEXT("----------"));
		UE_LOG(
			LogOpenUnrealUtilities,
			Log,
			TEXT("Tag, Source, IsExplicit, NumReferencersExclusive, NumReferencersInclusive"));

		TArray<TSharedPtr<FGameplayTagNode>> AllRootTags;
		UGameplayTagsManager::Get().GetFilteredGameplayRootTags(FString(), OUT AllRootTags);

		int32 NumExplicitTags = 0, NumImplicitTags = 0, NumReferencers = 0, NumUnreferencedTags = 0;
		TArray<int32> NumReferencersArray;
		for (auto& RootTagNode : AllRootTags)
		{
			int32 NumReferencersIncludingChildren = 0;
			PrintGameplayTagAndDepsRecursive(
				RootTagNode,
				IN OUT NumExplicitTags,
				IN OUT NumImplicitTags,
				IN OUT NumReferencers,
				IN OUT NumReferencersIncludingChildren,
				IN OUT NumUnreferencedTags,
				IN OUT NumReferencersArray);
		}

		const int32 AverageNumReferencers = NumReferencers / (NumExplicitTags + NumImplicitTags);

		const int32 HalfCount = IntCastChecked<int32>(NumReferencersArray.Num() / 2);
		NumReferencersArray.Sort();
		// We could try to get more precise by sampling the two middle values and averaging them, but who cares?
		const int32 MedianNumReferencers = NumReferencersArray[HalfCount];
		const int32 MinNumReferencers = NumReferencersArray[0];
		const int32 MaxNumReferencers = NumReferencersArray.Last();

		UE_LOG(LogOpenUnrealUtilities, Log, TEXT("----------"));
		UE_LOG(LogOpenUnrealUtilities, Log, TEXT("Gameplay tag stats:"));
		UE_LOG(LogOpenUnrealUtilities, Log, TEXT("Explicit tags:         %i"), NumExplicitTags);
		UE_LOG(LogOpenUnrealUtilities, Log, TEXT("Implicit tags:         %i"), NumImplicitTags);
		UE_LOG(LogOpenUnrealUtilities, Log, TEXT("Total tags:            %i"), NumExplicitTags + NumImplicitTags);
		UE_LOG(LogOpenUnrealUtilities, Log, TEXT("Total referencers:     %i"), NumReferencers);
		UE_LOG(LogOpenUnrealUtilities, Log, TEXT("Avg referencers:       %i"), AverageNumReferencers);
		UE_LOG(LogOpenUnrealUtilities, Log, TEXT("Med referencers:       %i"), MedianNumReferencers);
		UE_LOG(LogOpenUnrealUtilities, Log, TEXT("Min referencers:       %i"), MinNumReferencers);
		UE_LOG(LogOpenUnrealUtilities, Log, TEXT("Max referencers:       %i"), MaxNumReferencers);
		UE_LOG(LogOpenUnrealUtilities, Log, TEXT("Unused tags (no refs): %i"), NumUnreferencedTags);
		UE_LOG(LogOpenUnrealUtilities, Log, TEXT("----------"));
		UE_LOG(LogOpenUnrealUtilities, Log, TEXT("Gameplay Tag Audit END"));
	}

	static FAutoConsoleCommand PrintGameplayTagAuditCommand(
		TEXT("ouu.GameplayTagAudit"),
		TEXT("Print a gameplay tag audit to the output log"),
		FConsoleCommandDelegate::CreateStatic(PrintGameplayTagAudit));
} // namespace OUU::Editor::GameplayTagAudit
