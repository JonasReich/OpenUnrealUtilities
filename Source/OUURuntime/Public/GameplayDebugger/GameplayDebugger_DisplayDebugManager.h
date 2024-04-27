// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#if WITH_GAMEPLAY_DEBUGGER

class FGameplayDebuggerCanvasContext;

/**
 * Copy of FDisplayDebugManager that is DLL exported and therefore usable in gameplay debuggers.
 * Should be used sparingly, e.g. to convert existing ShowDebug commands into gameplay debuggers.
 */
struct OUURUNTIME_API FGameplayDebugger_DisplayDebugManager
{
public:
	explicit FGameplayDebugger_DisplayDebugManager(FGameplayDebuggerCanvasContext& InCanvasContext);

	void SetDrawColor(const FColor& NewColor);

	void SetLinearDrawColor(const FLinearColor& NewColor);

	void DrawString(const FString& InDebugString, const float& OptionalXOffset = 0.f);

	void AddColumnIfNeeded() const;

	float GetYStep() const;

	float GetXPos() const;

	float GetYPos() const;

	void SetYPos(const float NewYPos);

	float GetMaxCharHeight() const;

	void ShiftYDrawPosition(const float& YOffset);

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
					// If we only have one branch we treat it as the same really
					// as we may have only changed active status
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
	 * @param	Indent					Horizontal indent from left in pixels
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
		float& Indent,
		OnGetNumChildrenType OnGetNumChildren,
		OnGetChildType OnGetChildByIndex,
		OnGetDebugStringType OnGetDebugString)
	{
		TArray<FFlattenedDebugData> LineHelpers;
		int32 ChainId = 0;
		FFlattenedDebugData::RecursivelyFlattenDebugData(
			OUT LineHelpers,
			0,
			OUT ChainId,
			RootNode,
			OnGetNumChildren,
			OnGetChildByIndex,
			OnGetDebugString);

		DrawTree_Impl(LineHelpers, OUT Indent);
	}

private:
	void DrawTree_Impl(TArray<FFlattenedDebugData> LineHelpers, float& Indent);

	float NextColumnXPos = 0.f;
	FColor DrawColor;

	float MaxCursorY = 0.f;

	FGameplayDebuggerCanvasContext& CanvasContext;
};

#endif
