// Copyright (c) 2023 Jonas Reich & Contributors

#include "Animation/Debug/GameplayDebugger_Animation.h"

#if WITH_GAMEPLAY_DEBUGGER

	#include "Animation/AnimSequence.h"
	#include "Animation/AnimTypes.h"
	#include "Animation/Debug/DebuggableAnimInstance.h"
	#include "Components/SkeletalMeshComponent.h"
	#include "Components/StaticMeshComponent.h"
	#include "DisplayDebugHelpers.h"
	#include "DrawDebugHelpers.h"
	#include "Engine/Canvas.h"
	#include "Engine/Engine.h"
	#include "Engine/SkeletalMesh.h"
	#include "Engine/StaticMesh.h"
	#include "GameFramework/PlayerController.h"
	#include "GameplayDebugger/GameplayDebugger_TreeView.h"
	#include "Templates/StringUtils.h"

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
			DisplayDebug(CanvasContext, DebuggableAnimInstance, Canvas);
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
	FGameplayDebugger_TreeView TreeView{CanvasContext};
	TreeView.DrawTree(
		DebugActor->GetRootComponent(),
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
	UOUUDebuggableAnimInstance* AnimInstance,
	UCanvas* Canvas)
{
	namespace GameplayDebuggerSwitches = ::OUU::Runtime::Animation::GameplayDebugger::Private;

	const bool bShowSyncGroups = GetInputBoolSwitchValue(GameplayDebuggerSwitches::SyncGroups);
	const bool bShowMontages = GetInputBoolSwitchValue(GameplayDebuggerSwitches::Montages);
	const bool bShowGraph = GetInputBoolSwitchValue(GameplayDebuggerSwitches::Graph);
	const bool bShowCurves = GetInputBoolSwitchValue(GameplayDebuggerSwitches::Curves);
	const bool bShowNotifies = GetInputBoolSwitchValue(GameplayDebuggerSwitches::Notifies);
	const bool bFullGraph = GetInputBoolSwitchValue(GameplayDebuggerSwitches::FullGraphDisplay);
	const bool bFullBlendSpaceDisplay = GetInputBoolSwitchValue(GameplayDebuggerSwitches::FullBlendspaceDisplay);

	TArray<FName> ToggledCategories;

	// These are the names from UAnimInstance::DisplayDebug
	auto SwitchAnimDebugCat = [&](auto CategorySwitch, auto CategoryString, bool DefaultValueInAnimInstance) {
		if (CategorySwitch != DefaultValueInAnimInstance)
		{
			ToggledCategories.Add(CategoryString);
		}
	};

	SwitchAnimDebugCat(bShowSyncGroups, TEXT("SyncGroups"), true);
	SwitchAnimDebugCat(bShowMontages, TEXT("Montages"), true);
	SwitchAnimDebugCat(bShowGraph, TEXT("Graph"), true);
	SwitchAnimDebugCat(bShowCurves, TEXT("Curves"), true);
	SwitchAnimDebugCat(bShowNotifies, TEXT("Notifies"), true);
	SwitchAnimDebugCat(bFullGraph, TEXT("FullGraph"), false);
	SwitchAnimDebugCat(bFullBlendSpaceDisplay, TEXT("FullBlendspaceDisplay"), true);

	FDebugDisplayInfo DisplayInfo{TArray<FName>{}, ToggledCategories};

	float YL = CanvasContext.CursorY;
	float YPos = CanvasContext.CursorY;

	Canvas->DisplayDebugManager
		.Initialize(Canvas, GEngine->GetTinyFont(), {CanvasContext.CursorX, CanvasContext.CursorY});
	AnimInstance->DisplayDebug(Canvas, DisplayInfo, YL, YPos);
}

#endif
