// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#if WITH_GAMEPLAY_DEBUGGER

class FGameplayDebuggerCanvasContext;

/**
 * An extended version of FDisplayDebugManager that can be used to draw an expanded tree list of arbitrary data items to
 * the debug canvas, similar to the animation-graph view drawn by the "showdebug animation" command.
 */
struct OUURUNTIME_API FGameplayDebugger_TreeView
{
public:
	explicit FGameplayDebugger_TreeView(FGameplayDebuggerCanvasContext& InCanvasContext, FColor InDrawColor = FColor::White);

	struct FFlattenedDebugData
	{
		FString DebugString;
		int32 Indent = 0;
		int32 ChainID = 0;

		FFlattenedDebugData(const FString& InDebugString, int32 InIndent, int32 InChainID) :
			DebugString(InDebugString), Indent(InIndent), ChainID(InChainID)
		{
		}

		template <
			typename NodeType,
			typename OnGetNumChildrenType,
			typename OnGetChildType,
			typename OnGetDebugStringType>
		static void RecursivelyFlattenDebugData(
			TArray<FFlattenedDebugData>& FlattenedDebugData,
			int32 InIndent,
			int32& InChainID,
			NodeType* InNode,
			OnGetNumChildrenType OnGetNumChildren,
			OnGetChildType OnGetChildByIndex,
			OnGetDebugStringType OnGetDebugString)
		{
			const int32 CurChainID = InChainID;
			const int32 NumChildren = OnGetNumChildren(InNode);
			const bool bMultiBranch = NumChildren > 1;
			FlattenedDebugData.Add(FFlattenedDebugData{OnGetDebugString(InNode), InIndent, CurChainID});
			for (int32 i = 0; i < NumChildren; i++)
			{
				auto* ChildNode = OnGetChildByIndex(InNode, i);
				if (ChildNode == nullptr)
					continue;

				int32 ChildIndent = bMultiBranch ? InIndent + 1 : InIndent;
				if (bMultiBranch)
				{
					++InChainID;
				}
				RecursivelyFlattenDebugData(
					OUT FlattenedDebugData,
					ChildIndent,
					OUT InChainID,
					ChildNode,
					OnGetNumChildren,
					OnGetChildByIndex,
					OnGetDebugString);
			}
		}
	};

	/**
	 * Draw an expanded tree list of arbitrary data items to the debug canvas, similar to the animation-graph view
	 * drawn by the "showdebug animation" command.
	 *
	 * @param	RootNode				Root data item
	 * @param	OnGetNumChildren		Delegate to get the number of child items beneath a node
	 * @param	OnGetChildByIndex		Delegate to retrieve a child node from its parent node and
	 *									child index
	 * @param	OnGetDebugString		Delegate to get the debug string that is drawn for a node
	 *
	 * @tparam	NodeType				Node object that should be drawn to screen.
	 *									Must be sufficient to resolve child items and display text (see delegates)
	 * @tparam	OnGetNumChildrenType	int32(NodeType*)
	 * @tparam	OnGetChildType			NodeType*(NodeType*, int32)
	 * @tparam	OnGetDebugStringType	FString(NodeType*)
	 */
	template <typename NodeType, typename OnGetNumChildrenType, typename OnGetChildType, typename OnGetDebugStringType>
	void DrawTree(
		NodeType* RootNode,
		OnGetNumChildrenType OnGetNumChildren,
		OnGetChildType OnGetChildByIndex,
		OnGetDebugStringType OnGetDebugString)
	{
		TArray<FFlattenedDebugData> FlattenedDebugData;
		int32 ChainId = 0;
		FFlattenedDebugData::RecursivelyFlattenDebugData(
			OUT FlattenedDebugData,
			0,
			OUT ChainId,
			RootNode,
			OnGetNumChildren,
			OnGetChildByIndex,
			OnGetDebugString);

		DrawTree_Impl(FlattenedDebugData);
	}

private:
	void DrawString(const FString& InDebugString, const float& OptionalXOffset = 0.f);

	void AddColumnIfNeeded();

	float GetLineHeight() const;

	void DrawTree_Impl(TArray<FFlattenedDebugData> FlattenedDebugData);

	float NextColumnXPos = 0.f;
	FColor DrawColor;

	float MaxCursorY = 0.f;

	FGameplayDebuggerCanvasContext& CanvasContext;

	// The index represents indent level
	TArray<FVector2D> StartingPointsForIndentLevels;
};

#endif
