// Copyright (c) 2023 Jonas Reich & Contributors

#include "Animation/Debug/GameplayDebugger_Animation.h"

#if WITH_GAMEPLAY_DEBUGGER

	#include "Animation/AnimInstanceProxy.h"
	#include "Animation/AnimMontage.h"
	#include "Animation/AnimNodeBase.h"
	#include "Animation/AnimNotifies/AnimNotifyState.h"
	#include "Animation/AnimSequence.h"
	#include "Animation/AnimTypes.h"
	#include "Animation/Debug/DebuggableAnimInstance.h"
	#include "Components/SkeletalMeshComponent.h"
	#include "DisplayDebugHelpers.h"
	#include "DrawDebugHelpers.h"
	#include "Engine/Canvas.h"
	#include "Engine/Engine.h"
	#include "Engine/StaticMesh.h"
	#include "GameFramework/PlayerController.h"
	#include "GameplayDebugger/GameplayDebugger_DisplayDebugManager.h"
	#include "Misc/EngineVersionComparison.h"
	#include "Rendering/SkeletalMeshRenderData.h"
	#include "SkeletalRenderPublic.h"
	#include "Templates/StringUtils.h"
	#include "Animation/BlendSpace.h"

namespace OUU::Runtime::Animation::GameplayDebugger::Private
{
	FName SyncGroups = TEXT("Toggle Sync Groups");
	FName Montages = TEXT("Toggle Montages");
	FName Graph = TEXT("Toggle Graph");
	FName Curves = TEXT("Toggle Curves");
	FName Notifies = TEXT("Toggle Notifies");
	FName FullGraphDisplay = TEXT("Toggle Full Graph Display");
	FName FullBlendspaceDisplay = TEXT("Toggle Full Blendspace Display");
	FName SceneComponentTree = TEXT("Toggle Scene Component Tree");
} // namespace OUU::Runtime::Animation::GameplayDebugger::Private

FGameplayDebuggerCategory_Animation::FGameplayDebuggerCategory_Animation()
{
	BindKeyPress(
		TEXT("Cylce Debug Mesh"),
		EKeys::Insert.GetFName(),
		FGameplayDebuggerInputModifier::None,
		this,
		&FGameplayDebuggerCategory_Animation::CycleDebugMesh);

	BindKeyPress(
		TEXT("Cylce Debug Linked Instance"),
		EKeys::Delete.GetFName(),
		FGameplayDebuggerInputModifier::None,
		this,
		&FGameplayDebuggerCategory_Animation::CycleDebugInstance);

	#define BIND_SWITCH_KEY(Name, KeyName, DefaultValue)                                                               \
		BindKeyPress_Switch(                                                                                           \
			OUU::Runtime::Animation::GameplayDebugger::Private::Name,                                                  \
			EKeys::KeyName.GetFName(),                                                                                 \
			FGameplayDebuggerInputModifier::Ctrl,                                                                      \
			EGameplayDebuggerInputMode::Local,                                                                         \
			DefaultValue);

	BIND_SWITCH_KEY(SyncGroups, One, false);
	BIND_SWITCH_KEY(Montages, Two, true);
	BIND_SWITCH_KEY(Graph, Three, false);
	BIND_SWITCH_KEY(Curves, Four, false);
	BIND_SWITCH_KEY(Notifies, Five, true);
	BIND_SWITCH_KEY(FullGraphDisplay, Six, false);
	BIND_SWITCH_KEY(FullBlendspaceDisplay, Seven, false);
	BIND_SWITCH_KEY(SceneComponentTree, Eight, false);

	#undef BIND_SWITCH_KEY
}

void FGameplayDebuggerCategory_Animation::DrawData(
	APlayerController* OwnerPC,
	FGameplayDebuggerCanvasContext& CanvasContext)
{
	CanvasContext.FontRenderInfo.bEnableShadow = true;
	CanvasContext.Font = GEngine->GetSmallFont();

	PrintKeyBinds(CanvasContext);

	auto* Canvas = CanvasContext.Canvas.Get();
	if (!IsValid(Canvas))
		return;

	const auto* DebugActor = Cast<AActor>(FindLocalDebugActor());
	if (!IsValid(DebugActor))
		return;

	TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
	DebugActor->GetComponents<USkeletalMeshComponent>(OUT SkeletalMeshComponents);
	const int32 NumSkeletalMeshComponents = SkeletalMeshComponents.Num();
	if (NumSkeletalMeshComponents == 0)
		return;

	DebugMeshComponentIndex = DebugMeshComponentIndex % NumSkeletalMeshComponents;
	auto* DebugMeshComponent = SkeletalMeshComponents[DebugMeshComponentIndex];
	CanvasContext.Printf(
		TEXT("Target Mesh [%i / %i]: %s (%s)"),
		DebugMeshComponentIndex,
		NumSkeletalMeshComponents,
		*LexToString(DebugMeshComponent->GetSkeletalMeshAsset()),
		*LexToString(DebugMeshComponent));

	// cast to const USkeletalMeshComponent* is required so the right (public) overload of GetLinkedAnimInstances is
	// picked
	const TArray<UAnimInstance*>& LinkedAnimInstances =
		static_cast<const USkeletalMeshComponent*>(DebugMeshComponent)->GetLinkedAnimInstances();

	const int32 NumLinkedInstances = LinkedAnimInstances.Num();
	constexpr int32 Offset = 1;
	DebugInstanceIndex =
		NumLinkedInstances > 0 ? (((DebugInstanceIndex + Offset) % (NumLinkedInstances + Offset)) - Offset) : -1;

	if (GetInputBoolSwitchValue(OUU::Runtime::Animation::GameplayDebugger::Private::SceneComponentTree))
	{
		DrawSceneComponentTree(CanvasContext, DebugActor, DebugMeshComponent);
	}

	if (auto* AnimInstance =
			DebugInstanceIndex >= 0 ? LinkedAnimInstances[DebugInstanceIndex] : DebugMeshComponent->GetAnimInstance())
	{
		CanvasContext.Printf(
			TEXT("Target Anim Instance [%i / %i]: %s"),
			DebugInstanceIndex,
			NumLinkedInstances,
			*LexToString(AnimInstance));

		if (auto* DebuggableAnimInstance = Cast<UOUUDebuggableAnimInstance>(AnimInstance))
		{
			DisplayDebug(CanvasContext, DebugMeshComponent, DebuggableAnimInstance, Canvas);
		}
		else
		{
			CanvasContext.Printf(
				TEXT("{yellow} Anim instance %s is not a UOUUDebuggableAnimInstance, so we cannot display any data"),
				*AnimInstance->GetName());
		}
	}
}

void FGameplayDebuggerCategory_Animation::CycleDebugMesh()
{
	DebugMeshComponentIndex++;
}

void FGameplayDebuggerCategory_Animation::CycleDebugInstance()
{
	DebugInstanceIndex++;
}

void FGameplayDebuggerCategory_Animation::DrawSceneComponentTree(
	FGameplayDebuggerCanvasContext& CanvasContext,
	const AActor* DebugActor,
	const USkeletalMeshComponent* DebugMeshComponent)
{
	float Indent = 0;

	FGameplayDebugger_DisplayDebugManager DisplayDebugManager{CanvasContext};
	DisplayDebugManager.DrawTree(
		DebugActor->GetRootComponent(),
		OUT Indent,
		// GetNumNodeChildren
		[](const USceneComponent* SceneComp) -> int32 { return SceneComp->GetNumChildrenComponents(); },
		// OnGetChildByIndex
		[](const USceneComponent* SceneComponent, int32 ChildIdx) -> USceneComponent* {
			return SceneComponent->GetChildComponent(ChildIdx);
		},
		// OnGetDebugString
		[&](USceneComponent* SceneComponent) -> FString {
			const FName SocketName = SceneComponent->GetAttachSocketName();
			const FString SocketNameString = (SocketName == NAME_None ? FString("") : SocketName.ToString());
			const FString ColorString = SceneComponent == DebugMeshComponent ? "{green}" : "{white}";
			const FString SceneComponentName = SceneComponent->GetName();
			FString OptionalMeshString = "";
			if (const auto* StaticMeshComponent = Cast<UStaticMeshComponent>(SceneComponent))
			{
				OptionalMeshString = FString::Printf(TEXT("(%s)"), *LexToString(StaticMeshComponent->GetStaticMesh()));
			}
			else if (const auto* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(SceneComponent))
			{
				OptionalMeshString =
					FString::Printf(TEXT("(%s)"), *LexToString(SkeletalMeshComponent->GetSkeletalMeshAsset()));
			}

			return FString::Printf(
				TEXT("[{yellow}%s{white}] %s%s %s\n"
					 "\t\t{grey}T|R|S = %s"),
				*SocketNameString,
				*ColorString,
				*SceneComponentName,
				*OptionalMeshString,
				*SceneComponent->GetRelativeTransform().ToString());
		});
}

void FGameplayDebuggerCategory_Animation::DisplayDebug(
	FGameplayDebuggerCanvasContext& CanvasContext,
	USkeletalMeshComponent* SkeletalMeshComponent,
	UOUUDebuggableAnimInstance* AnimInstance,
	UCanvas* Canvas)
{
	FOUUDebuggableAnimInstanceProxy& Proxy = AnimInstance->GetProxyOnGameThread<FOUUDebuggableAnimInstanceProxy>();

	float Indent = 0.f;

	FLinearColor TextYellow = FColorList::Yellow;
	FLinearColor TextWhite = FColorList::White;
	FLinearColor ActiveColor = FColorList::YellowGreen;
	FLinearColor InactiveColor = FColorList::LightGrey;
	FLinearColor PoseSourceColor = FColorList::VioletRed;

	namespace GameplayDebuggerSwitches = ::OUU::Runtime::Animation::GameplayDebugger::Private;

	const bool bShowSyncGroups = GetInputBoolSwitchValue(GameplayDebuggerSwitches::SyncGroups);
	const bool bShowMontages = GetInputBoolSwitchValue(GameplayDebuggerSwitches::Montages);
	const bool bShowGraph = GetInputBoolSwitchValue(GameplayDebuggerSwitches::Graph);
	const bool bShowCurves = GetInputBoolSwitchValue(GameplayDebuggerSwitches::Curves);
	const bool bShowNotifies = GetInputBoolSwitchValue(GameplayDebuggerSwitches::Notifies);
	const bool bFullGraph = GetInputBoolSwitchValue(GameplayDebuggerSwitches::FullGraphDisplay);
	const bool bFullBlendSpaceDisplay = GetInputBoolSwitchValue(GameplayDebuggerSwitches::FullBlendspaceDisplay);

	FString Heading = FString::Printf(TEXT("Animation: %s"), *AnimInstance->GetName());

	FGameplayDebugger_DisplayDebugManager DisplayDebugManager{CanvasContext};

	DisplayDebugManager.SetLinearDrawColor(TextYellow);
	DisplayDebugManager.DrawString(Heading, Indent);

	{
		FIndenter CustomDebugIndent(Indent);
		DisplayDebugInstance(DisplayDebugManager, SkeletalMeshComponent, AnimInstance, Indent);

		AnimInstance->AddGameplayDebuggerInfo(CanvasContext);
	}

	if (bShowGraph && Proxy.HasRootNode())
	{
		DisplayDebugManager.SetLinearDrawColor(TextYellow);

		Heading = FString::Printf(TEXT("Anim Node Tree"));
		DisplayDebugManager.DrawString(Heading, Indent);

		constexpr float NodeIndent = 8.f;
		constexpr float LineIndent = 4.f;
		constexpr float AttachLineLength = NodeIndent - LineIndent;

		FIndenter AnimNodeTreeIndent(Indent);

		DebugDataCounter.Increment();
		FNodeDebugData NodeDebugData(AnimInstance);
		Proxy.GatherDebugData(NodeDebugData);

		TArray<FNodeDebugData::FFlattenedDebugData> FlattenedData = NodeDebugData.GetFlattenedDebugData();

		// Index represents indent level, track the current starting point for that
		TArray<FVector2D> IndentLineStartCoord;

		int32 PrevChainID = -1;

		for (FNodeDebugData::FFlattenedDebugData& Line : FlattenedData)
		{
			if (!Line.IsOnActiveBranch() && !bFullGraph)
			{
				continue;
			}
			float CurrIndent = Indent + (Line.Indent * NodeIndent);
			float CurrLineYBase = DisplayDebugManager.GetYPos() + DisplayDebugManager.GetMaxCharHeight();

			if (PrevChainID != Line.ChainID)
			{
				const int32 HalfStep = static_cast<int32>(DisplayDebugManager.GetMaxCharHeight() / 2);
				DisplayDebugManager.ShiftYDrawPosition(
					static_cast<float>(HalfStep)); // Extra spacing to delimit different chains, CurrLineYBase now
				// roughly represents middle of text line, so we can use it for line drawing

				// Handle line drawing
				int32 VerticalLineIndex = Line.Indent - 1;
				if (IndentLineStartCoord.IsValidIndex(VerticalLineIndex))
				{
					FVector2D LineStartCoord = IndentLineStartCoord[VerticalLineIndex];
					IndentLineStartCoord[VerticalLineIndex] = FVector2D(DisplayDebugManager.GetXPos(), CurrLineYBase);

					// If indent parent is not in same column, ignore line.
					if (FMath::IsNearlyEqual(LineStartCoord.X, DisplayDebugManager.GetXPos()))
					{
						float EndX = DisplayDebugManager.GetXPos() + CurrIndent;
						float StartX = EndX - AttachLineLength;

						// horizontal line to node
						DrawDebugCanvas2DLine(
							Canvas,
							FVector(StartX, CurrLineYBase, 0.f),
							FVector(EndX, CurrLineYBase, 0.f),
							ActiveColor);

						// vertical line
						DrawDebugCanvas2DLine(
							Canvas,
							FVector(StartX, LineStartCoord.Y, 0.f),
							FVector(StartX, CurrLineYBase, 0.f),
							ActiveColor);
					}
				}

				CurrLineYBase += HalfStep; // move CurrYLineBase back to base of line
			}

			// Update our base position for subsequent line drawing
			if (!IndentLineStartCoord.IsValidIndex(Line.Indent))
			{
				IndentLineStartCoord.AddZeroed(Line.Indent + 1 - IndentLineStartCoord.Num());
			}
			IndentLineStartCoord[Line.Indent] = FVector2D(DisplayDebugManager.GetXPos(), CurrLineYBase);

			PrevChainID = Line.ChainID;
			FLinearColor ItemColor = Line.bPoseSource ? PoseSourceColor : ActiveColor;
			DisplayDebugManager.SetLinearDrawColor(Line.IsOnActiveBranch() ? ItemColor : InactiveColor);
			DisplayDebugManager.DrawString(Line.DebugLine, CurrIndent);
		}
	}

	if (bShowSyncGroups)
	{
		FIndenter AnimIndent(Indent);

		// Display Sync Groups
		const FAnimInstanceProxy::FSyncGroupMap& SyncGroupMap = Proxy.GetSyncGroupMapRead();
		const TArray<FAnimTickRecord>& UngroupedActivePlayers = Proxy.GetUngroupedActivePlayersRead();

		Heading = FString::Printf(TEXT("SyncGroups: %i"), SyncGroupMap.Num());
		DisplayDebugManager.DrawString(Heading, Indent);

		for (const auto& SyncGroupPair : SyncGroupMap)
		{
			FIndenter GroupIndent(Indent);
			const FAnimGroupInstance& SyncGroup = SyncGroupPair.Value;

			DisplayDebugManager.SetLinearDrawColor(TextYellow);

			FString GroupLabel = FString::Printf(
				TEXT("Group %s - Players %i"),
				*SyncGroupPair.Key.ToString(),
				SyncGroup.ActivePlayers.Num());
			DisplayDebugManager.DrawString(GroupLabel, Indent);

			if (SyncGroup.ActivePlayers.Num() > 0)
			{
				check(SyncGroup.GroupLeaderIndex != -1);
				OutputTickRecords(
					SyncGroup.ActivePlayers,
					Canvas,
					Indent,
					SyncGroup.GroupLeaderIndex,
					TextWhite,
					ActiveColor,
					InactiveColor,
					DisplayDebugManager,
					bFullBlendSpaceDisplay);
			}
		}

		DisplayDebugManager.SetLinearDrawColor(TextYellow);

		Heading = FString::Printf(TEXT("Ungrouped: %i"), UngroupedActivePlayers.Num());
		DisplayDebugManager.DrawString(Heading, Indent);

		DisplayDebugManager.SetLinearDrawColor(TextWhite);

		OutputTickRecords(
			UngroupedActivePlayers,
			Canvas,
			Indent,
			-1,
			TextWhite,
			ActiveColor,
			InactiveColor,
			DisplayDebugManager,
			bFullBlendSpaceDisplay);
	}

	if (bShowMontages)
	{
		DisplayDebugManager.SetLinearDrawColor(TextYellow);

		Heading = FString::Printf(TEXT("Montages: %i"), AnimInstance->MontageInstances.Num());
		DisplayDebugManager.DrawString(Heading, Indent);

		for (int32 MontageIndex = 0; MontageIndex < AnimInstance->MontageInstances.Num(); ++MontageIndex)
		{
			FIndenter PlayerIndent(Indent);

			FAnimMontageInstance* MontageInstance = AnimInstance->MontageInstances[MontageIndex];

			DisplayDebugManager.SetLinearDrawColor((MontageInstance->IsActive()) ? ActiveColor : TextWhite);

			FString MontageEntry = FString::Printf(
				TEXT("%i) %s CurrSec: %s NextSec: %s W:%.2f DW:%.2f"),
				MontageIndex,
				*MontageInstance->Montage->GetName(),
				*MontageInstance->GetCurrentSection().ToString(),
				*MontageInstance->GetNextSection().ToString(),
				MontageInstance->GetWeight(),
				MontageInstance->GetDesiredWeight());
			DisplayDebugManager.DrawString(MontageEntry, Indent);
		}
	}

	if (bShowNotifies)
	{
		DisplayDebugManager.SetLinearDrawColor(TextYellow);

		Heading = FString::Printf(TEXT("Active Notify States: %i"), AnimInstance->ActiveAnimNotifyState.Num());
		DisplayDebugManager.DrawString(Heading, Indent);

		DisplayDebugManager.SetLinearDrawColor(TextWhite);

		for (int32 NotifyIndex = 0; NotifyIndex < AnimInstance->ActiveAnimNotifyState.Num(); ++NotifyIndex)
		{
			FIndenter NotifyIndent(Indent);

			const FAnimNotifyEvent& NotifyState = AnimInstance->ActiveAnimNotifyState[NotifyIndex];

			FString NotifyEntry = FString::Printf(
				TEXT("%i) %s Class: %s Dur:%.3f"),
				NotifyIndex,
				*NotifyState.NotifyName.ToString(),
				*NotifyState.NotifyStateClass->GetName(),
				NotifyState.GetDuration());
			DisplayDebugManager.DrawString(NotifyEntry, Indent);
		}
	}

	if (bShowCurves)
	{
		DisplayDebugManager.SetLinearDrawColor(TextYellow);

		Heading = FString::Printf(TEXT("Curves"));
		DisplayDebugManager.DrawString(Heading, Indent);

		{
			FIndenter CurveIndent(Indent);

			Heading = FString::Printf(
				TEXT("Morph Curves: %i"),
				Proxy.GetAnimationCurves(EAnimCurveType::MorphTargetCurve).Num());
			DisplayDebugManager.DrawString(Heading, Indent);

			DisplayDebugManager.SetLinearDrawColor(TextWhite);

			{
				FIndenter MorphCurveIndent(Indent);
				OutputCurveMap(Proxy.GetAnimationCurves(EAnimCurveType::MorphTargetCurve), DisplayDebugManager, Indent);
			}

			DisplayDebugManager.SetLinearDrawColor(TextYellow);

			Heading = FString::Printf(
				TEXT("Material Curves: %i"),
				Proxy.GetAnimationCurves(EAnimCurveType::MaterialCurve).Num());
			DisplayDebugManager.DrawString(Heading, Indent);

			DisplayDebugManager.SetLinearDrawColor(TextWhite);

			{
				FIndenter MaterialCurveIndent(Indent);
				OutputCurveMap(Proxy.GetAnimationCurves(EAnimCurveType::MaterialCurve), DisplayDebugManager, Indent);
			}

			DisplayDebugManager.SetLinearDrawColor(TextYellow);

			Heading = FString::Printf(
				TEXT("Event Curves: %i"),
				Proxy.GetAnimationCurves(EAnimCurveType::AttributeCurve).Num());
			DisplayDebugManager.DrawString(Heading, Indent);

			DisplayDebugManager.SetLinearDrawColor(TextWhite);

			{
				FIndenter EventCurveIndent(Indent);
				OutputCurveMap(Proxy.GetAnimationCurves(EAnimCurveType::AttributeCurve), DisplayDebugManager, Indent);
			}
		}
	}
}

void FGameplayDebuggerCategory_Animation::DisplayDebugInstance(
	FGameplayDebugger_DisplayDebugManager& DisplayDebugManager,
	const USkeletalMeshComponent* SkeletalMeshComponent,
	UOUUDebuggableAnimInstance* AnimInstance,
	const float& Indent)
{
	const auto* ProxyPtr =
		UOUUDebuggableAnimInstance::GetProxyOnGameThreadStatic<FOUUDebuggableAnimInstanceProxy>(AnimInstance);
	if (!ProxyPtr)
	{
		DisplayDebugManager.DrawString(
			FString::Printf(TEXT(
				"Anim instance does not have proxy of type OUUDebuggableAnimInstanceProxy and cannot be debugged")),
			Indent);
		return;
	}

	DisplayDebugManager.SetLinearDrawColor(FLinearColor::Green);

	const int32 MaxLODIndex = SkeletalMeshComponent->MeshObject
		? (SkeletalMeshComponent->MeshObject->GetSkeletalMeshRenderData().LODRenderData.Num() - 1)
		: INDEX_NONE;

	const FOUUDebuggableAnimInstanceProxy& Proxy = *ProxyPtr;

	FString DebugText = FString::Printf(
		TEXT("LOD(%d/%d) UpdateCounter(%d) EvalCounter(%d) CacheBoneCounter(%d) InitCounter(%d) DeltaSeconds(%.3f)"),
		SkeletalMeshComponent->GetPredictedLODLevel(),
		MaxLODIndex,
		Proxy.GetUpdateCounter().Get(),
		Proxy.GetEvaluationCounter().Get(),
		Proxy.GetCachedBonesCounter().Get(),
		Proxy.GetInitializationCounter().Get(),
		Proxy.GetDeltaSeconds());

	DisplayDebugManager.DrawString(DebugText, Indent);

	if (SkeletalMeshComponent->ShouldUseUpdateRateOptimizations())
	{
		if (const FAnimUpdateRateParameters* UROParams = SkeletalMeshComponent->AnimUpdateRateParams)
		{
			DebugText = FString::Printf(
				TEXT("URO Rate(%d) SkipUpdate(%d) SkipEval(%d) Interp(%d)"),
				UROParams->UpdateRate,
				UROParams->ShouldSkipUpdate(),
				UROParams->ShouldSkipEvaluation(),
				UROParams->ShouldInterpolateSkippedFrames());

			DisplayDebugManager.DrawString(DebugText, Indent);
		}
	}
}

void FGameplayDebuggerCategory_Animation::OutputTickRecords(
	const TArray<FAnimTickRecord>& Records,
	UCanvas* Canvas,
	float Indent,
	const int32 HighlightIndex,
	FLinearColor TextColor,
	FLinearColor HighlightColor,
	FLinearColor InactiveColor,
	FGameplayDebugger_DisplayDebugManager& DisplayDebugManager,
	bool bFullBlendSpaceDisplay)
{
	for (int32 PlayerIndex = 0; PlayerIndex < Records.Num(); ++PlayerIndex)
	{
		const FAnimTickRecord& Player = Records[PlayerIndex];

		DisplayDebugManager.SetLinearDrawColor((PlayerIndex == HighlightIndex) ? HighlightColor : TextColor);

		FString PlayerEntry = FString::Printf(
			TEXT("%i) %s (%s) W(%.f%%)"),
			PlayerIndex,
			*Player.SourceAsset->GetName(),
			*Player.SourceAsset->GetClass()->GetName(),
			Player.EffectiveBlendWeight * 100.f);

		// See if we have access to SequenceLength
		if (UAnimSequenceBase* AnimSeqBase = Cast<UAnimSequenceBase>(Player.SourceAsset))
		{
			float SequenceLength = AnimSeqBase->GetPlayLength();
			PlayerEntry += FString::Printf(
				TEXT(" P(%.2f/%.2f)"),
				Player.TimeAccumulator != nullptr ? *Player.TimeAccumulator : 0.f,
				SequenceLength);
		}
		else
		{
			PlayerEntry +=
				FString::Printf(TEXT(" P(%.2f)"), Player.TimeAccumulator != nullptr ? *Player.TimeAccumulator : 0.f);
		}

		// Part of a sync group
		if (HighlightIndex != INDEX_NONE)
		{
			PlayerEntry += FString::Printf(
				TEXT(" Prev(i:%d, t:%.3f) Next(i:%d, t:%.3f)"),
				Player.MarkerTickRecord->PreviousMarker.MarkerIndex,
				Player.MarkerTickRecord->PreviousMarker.TimeToMarker,
				Player.MarkerTickRecord->NextMarker.MarkerIndex,
				Player.MarkerTickRecord->NextMarker.TimeToMarker);
		}

		DisplayDebugManager.DrawString(PlayerEntry, Indent);

		if (UBlendSpace* BlendSpace = Cast<UBlendSpace>(Player.SourceAsset))
		{
			if (bFullBlendSpaceDisplay && Player.BlendSpace.BlendSampleDataCache
				&& Player.BlendSpace.BlendSampleDataCache->Num() > 0)
			{
				TArray<FBlendSampleData> SampleData = *Player.BlendSpace.BlendSampleDataCache;
				SampleData.Sort([](const FBlendSampleData& L, const FBlendSampleData& R) {
					return L.SampleDataIndex < R.SampleDataIndex;
				});

				FIndenter BlendSpaceIndent(Indent);
				const FVector BlendSpacePosition(
					Player.BlendSpace.BlendSpacePositionX,
					Player.BlendSpace.BlendSpacePositionY,
					0.f);
				FString BlendSpaceHeader =
					FString::Printf(TEXT("Blendspace Input (%s)"), *BlendSpacePosition.ToString());
				DisplayDebugManager.DrawString(BlendSpaceHeader, Indent);

				const TArray<FBlendSample>& BlendSamples = BlendSpace->GetBlendSamples();

				int32 WeightedSampleIndex = 0;

				for (int32 SampleIndex = 0; SampleIndex < BlendSamples.Num(); ++SampleIndex)
				{
					const FBlendSample& BlendSample = BlendSamples[SampleIndex];

					float Weight = 0.f;
					for (; WeightedSampleIndex < SampleData.Num(); ++WeightedSampleIndex)
					{
						FBlendSampleData& WeightedSample = SampleData[WeightedSampleIndex];
						if (WeightedSample.SampleDataIndex == SampleIndex)
						{
							Weight += WeightedSample.GetClampedWeight();
						}
						else if (WeightedSample.SampleDataIndex > SampleIndex)
						{
							break;
						}
					}

					FIndenter SampleIndent(Indent);

					DisplayDebugManager.SetLinearDrawColor((Weight > 0.f) ? TextColor : InactiveColor);

					FString SampleEntry =
						FString::Printf(TEXT("%s W:%.1f%%"), *BlendSample.Animation->GetName(), Weight * 100.f);
					DisplayDebugManager.DrawString(SampleEntry, Indent);
				}
			}
		}
	}
}

void FGameplayDebuggerCategory_Animation::OutputCurveMap(
	TMap<FName, float>& CurveMap,
	FGameplayDebugger_DisplayDebugManager& DisplayDebugManager,
	float Indent)
{
	TArray<FName> Names;
	CurveMap.GetKeys(Names);
	Names.Sort(FNameLexicalLess());
	for (FName CurveName : Names)
	{
		FString CurveEntry = FString::Printf(TEXT("%s: %.3f"), *CurveName.ToString(), CurveMap[CurveName]);
		DisplayDebugManager.DrawString(CurveEntry, Indent);
	}
}

#endif
