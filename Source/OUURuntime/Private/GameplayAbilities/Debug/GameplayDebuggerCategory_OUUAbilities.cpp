// Copyright (c) 2023 Jonas Reich & Contributors

#include "GameplayAbilities/Debug/GameplayDebuggerCategory_OUUAbilities.h"

#if WITH_GAMEPLAY_DEBUGGER

	#include "Misc/EngineVersionComparison.h"
	#include "TimerManager.h"
	#include "AbilitySystemComponent.h"
	#include "AbilitySystemGlobals.h"
	#include "Engine/Canvas.h"
	#include "GameplayAbilities/OUUAbilitySystemComponent.h"
	#include "GameplayAbilities/OUUGameplayAbility.h"
	#include "GameplayAbilitySpec.h"
	#include "GameplayCueManager.h"
	#include "GameplayCueNotify_Actor.h"
	#include "GameplayCueNotify_Static.h"
	#include "GameplayCueSet.h"
	#include "GameplayDebugger/GameplayDebuggerUtils.h"
	#include "GameplayEffect.h"
	#include "GameplayTagContainer.h"
	#include "GameplayTagsManager.h"
	#include "Misc/RegexUtils.h"
	#include "Templates/ReverseIterator.h"
	#include "Templates/StringUtils.h"
	#include "Engine.h"

TAutoConsoleVariable<float>
	CVar_NumColumns{TEXT("ouu.Debug.Ability.NumColumns"), 4.f, TEXT("How many columns the ability debugger may use.")};

TAutoConsoleVariable<FString> CVar_AbilityFilter{
	TEXT("ouu.Debug.Ability.Filter"),
	TEXT(".*"),
	TEXT("Regular expression filter for ability names. Default value: '.*' (allow all).")};

bool bPrintNotLoadedCues = false;
FAutoConsoleVariableRef CVar_bPrintNotLoadedCues{
	TEXT("ouu.Debug.Ability.PrintNotLoadedCues"),
	bPrintNotLoadedCues,
	TEXT("Should unloaded gameplay cues be printed? (Default: false)")};
bool bPrintUnmappedCues = false;
FAutoConsoleVariableRef CVar_bPrintUnmappedCues{
	TEXT("ouu.Debug.Ability.PrintUnmappedCues"),
	bPrintUnmappedCues,
	TEXT("Should unmapped gameplay cues be printed? (Default: false)")};

void FGameplayDebuggerCategory_OUUAbilities::DrawBackground(
	FGameplayDebuggerCanvasContext& CanvasContext,
	const FVector2D& BackgroundLocation,
	const FVector2D& BackgroundSize)
{
	constexpr FLinearColor BackgroundColor(0.1f, 0.1f, 0.1f, 0.8f);
	FCanvasTileItem Background(FVector2D(0.0f, 0.0f), BackgroundSize, BackgroundColor);
	Background.BlendMode = SE_BLEND_Translucent;
	CanvasContext.DrawItem(Background, BackgroundLocation.X, BackgroundLocation.Y);
}

void FGameplayDebuggerCategory_OUUAbilities::DrawData(
	APlayerController* OwnerPC,
	FGameplayDebuggerCanvasContext& CanvasContext)
{
	CanvasContext.Font = GEngine->GetTinyFont();
	CanvasContext.CursorY += 10.f;
	Canvas = CanvasContext.Canvas.Get();

	auto* AbilityComp = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(FindLocalDebugActor());
	if (AbilityComp == nullptr || Canvas == nullptr)
		return;

	AbilitySystem = Cast<UOUUAbilitySystemComponent>(AbilityComp);
	if (AbilitySystem == nullptr)
	{
		CanvasContext.Printf(
			TEXT("{yellow}Ability system component %s (%s) is not a UOUUAbilitySystemComponent.\nPlease "
				 "use the regular ability system debugger or reparent your component class."),
			*AbilityComp->GetName(),
			*AbilityComp->GetClass()->GetName());
		return;
	}
	FVector2D ViewPortSize;
	GEngine->GameViewport->GetViewportSize(OUT ViewPortSize);
	const float RemainingViewportHeight = ViewPortSize.Y - CanvasContext.CursorY;

	// Reset debug info for this frame
	DebugInfo = {};

	DebugInfo.XPos = CanvasContext.CursorX;
	DebugInfo.YPos = CanvasContext.CursorY;
	DebugInfo.MinX = CanvasContext.CursorX;

	constexpr float BackgroundPadding = 5.0f;
	// Add some extra padding on the bottom.
	// It's likely the task bar overlaps or some other clipping issues cause content to get lost here otherwise.
	DebugInfo.MaxY = ViewPortSize.Y - BackgroundPadding - 50.f;
	// Also add some padding on the top
	DebugInfo.MinY = CanvasContext.CursorY + BackgroundPadding + 25.f;

	const FVector2D BackgroundLocation = FVector2D(BackgroundPadding, CanvasContext.CursorY - BackgroundPadding);
	const FVector2D BackgroundSize(
		ViewPortSize.X - 2 * BackgroundPadding,
		RemainingViewportHeight - 2 * BackgroundPadding);

	DrawBackground(CanvasContext, BackgroundLocation, BackgroundSize);
	DrawDebugHeader();
	DrawDebugBody();

	// We use up the entire screen with background, but some internal code may set the cursor to non-final location.
	// This asserts that further debuggers don't accidentally draw over this one.
	CanvasContext.CursorY = ViewPortSize.Y;
}

void FGameplayDebuggerCategory_OUUAbilities::DebugDrawGameplayEffectModifier(
	FActiveGameplayEffect& ActiveGE,
	const FModifierSpec& ModSpec,
	const FGameplayModifierInfo& ModInfo)
{
	// ReSharper disable once CppTooWideScope
	const bool IsZeroChangeModifier =
		(ModInfo.ModifierOp == EGameplayModOp::Additive && ModSpec.GetEvaluatedMagnitude() == 0.f);
	if (IsZeroChangeModifier)
		return;

	Canvas->SetDrawColor(ActiveGE.bIsInhibited ? FColor(128, 128, 128) : FColor::White);
	DebugLine(
		FString::Printf(
			TEXT("Mod: %s, %s, %.2f"),
			*ModInfo.Attribute.GetName(),
			*EGameplayModOpToString(ModInfo.ModifierOp),
			ModSpec.GetEvaluatedMagnitude()),
		14.f,
		0);

	const FString SourceTagString = ModInfo.SourceTags.ToString();

	if (SourceTagString.Len() > 0)
	{
		if (SourceTagString.Len() <= 50)
		{
			DebugLine(FString::Printf(TEXT("SourceTags: %s"), *SourceTagString), 9.f, 0);
		}
		else
		{
			DebugLine(FString::Printf(TEXT("SourceTags: %s"), *SourceTagString.Left(50)), 9.f, 0);
			DebugLine(SourceTagString.RightChop(50), 10.f, 0);
		}
	}
}

void FGameplayDebuggerCategory_OUUAbilities::DrawGameplayEffect(FActiveGameplayEffect& ActiveGE)
{
	FString DurationStr = TEXT("Infinite Duration ");
	if (ActiveGE.GetDuration() > 0.f)
	{
		DurationStr = FString::Printf(
			TEXT("Duration: %.2f. Remaining: %.2f (Start: %.2f / %.2f / %.2f) %s "),
			ActiveGE.GetDuration(),
			ActiveGE.GetTimeRemaining(AbilitySystem->GetWorld()->GetTimeSeconds()),
			ActiveGE.StartServerWorldTime,
			ActiveGE.CachedStartServerWorldTime,
			ActiveGE.StartWorldTime,
			ActiveGE.DurationHandle.IsValid() ? TEXT("Valid Handle") : TEXT("INVALID Handle"));
		if (ActiveGE.DurationHandle.IsValid())
		{
			DurationStr += FString::Printf(
				TEXT("(Local Duration: %.2f)"),
				AbilitySystem->GetWorld()->GetTimerManager().GetTimerRemaining(ActiveGE.DurationHandle));
		}
	}
	if (ActiveGE.GetPeriod() > 0.f)
	{
		DurationStr += FString::Printf(TEXT("Period: %.2f"), ActiveGE.GetPeriod());
	}

	FString StackString;
	#if UE_VERSION_OLDER_THAN(5, 3, 0)
	const int32 ActiveGE_StackCount = ActiveGE.Spec.GetStackCount();
	#else
	const int32 ActiveGE_StackCount = ActiveGE.Spec.GetStackCount();
	#endif
	if (ActiveGE_StackCount > 1)
	{
		if (ActiveGE.Spec.Def->GetStackingType() == EGameplayEffectStackingType::AggregateBySource)
		{
			StackString = FString::Printf(
				TEXT("(Stacks: %d. From: %s) "),
				ActiveGE_StackCount,
				*GetNameSafe(
					ActiveGE.Spec.GetContext().GetInstigatorAbilitySystemComponent()->GetAvatarActor_Direct()));
		}
		else
		{
			StackString = FString::Printf(TEXT("(Stacks: %d) "), ActiveGE_StackCount);
		}
	}

	FString LevelString;
	if (ActiveGE.Spec.GetLevel() > 1.f)
	{
		LevelString = FString::Printf(TEXT("Level: %.2f"), ActiveGE.Spec.GetLevel());
	}

	FString PredictionString;
	if (ActiveGE.PredictionKey.IsValidKey())
	{
		PredictionString = ActiveGE.PredictionKey.WasLocallyGenerated() ? TEXT("(Predicted and Waiting)")
																		: TEXT("(Predicted and Caught Up)");
	}

	Canvas->SetDrawColor(ActiveGE.bIsInhibited ? FColorList::Grey : FColor::Emerald);

	DebugLine(
		FString::Printf(
			TEXT("%s %s %s %s %s"),
			*OUU::Runtime::GameplayDebuggerUtils::CleanupName(GetNameSafe(ActiveGE.Spec.Def)),
			*DurationStr,
			*StackString,
			*LevelString,
			*PredictionString),
		4.f,
		0);

	Canvas->SetDrawColor(ActiveGE.bIsInhibited ? FColorList::Grey : FColor::White);

	FGameplayTagContainer GrantedTags;
	ActiveGE.Spec.GetAllGrantedTags(GrantedTags);
	if (GrantedTags.Num() > 0)
	{
		DebugLine(FString::Printf(TEXT("Granted Tags: %s"), *GrantedTags.ToStringSimple()), 14.f, 0);
	}

	for (int32 ModIdx = 0; ModIdx < ActiveGE.Spec.Modifiers.Num(); ++ModIdx)
	{
		if (ActiveGE.Spec.Def == nullptr)
			continue;

		const FModifierSpec& ModSpec = ActiveGE.Spec.Modifiers[ModIdx];
		const FGameplayModifierInfo& ModInfo = ActiveGE.Spec.Def->Modifiers[ModIdx];
		DebugDrawGameplayEffectModifier(ActiveGE, ModSpec, ModInfo);
	}
}

void FGameplayDebuggerCategory_OUUAbilities::DrawGameplayAbilityInstance(UOUUGameplayAbility* Instance)
{
	Canvas->SetDrawColor(FColor::White);
	for (UGameplayTask* Task : Instance->ActiveTasks)
	{
		if (Task == nullptr)
			continue;

		DebugLine(Task->GetDebugString(), 7.f, 0);

		for (FAbilityTaskDebugMessage& Msg : Instance->TaskDebugMessages)
		{
			if (Msg.FromTask == Task)
			{
				DebugLine(Msg.Message, 9.f, 0);
			}
		}
	}

	bool FirstTaskMsg = true;
	int32 MsgCount = 0;
	constexpr int32 MaskTaskDebugCount = 5;
	for (FAbilityTaskDebugMessage& Msg : ReverseRange(Instance->TaskDebugMessages))
	{
		if (Instance->ActiveTasks.Contains(Msg.FromTask) == false)
		{
			// Cap finished task messages to 5 per ability if we are printing to screen (else things
			// will scroll off)
			if (++MsgCount > MaskTaskDebugCount)
			{
				break;
			}

			if (FirstTaskMsg)
			{
				DebugLine(
					FString::Printf(
						TEXT("[FinishedTasks (last %i of %i)]"),
						FMath::Min<int32>(Instance->TaskDebugMessages.Num(), MaskTaskDebugCount),
						Instance->TaskDebugMessages.Num()),
					7.f,
					0);
				FirstTaskMsg = false;
			}

			DebugLine(Msg.Message, 9.f, 0);
		}
	}
}

void FGameplayDebuggerCategory_OUUAbilities::DetermineAbilityStatusText(
	const FGameplayTagContainer& BlockedAbilityTags,
	const FGameplayAbilitySpec& AbilitySpec,
	const UGameplayAbility* Ability,
	FString& OutStatusText,
	FColor& OutAbilityTextColor) const
{
	FGameplayTagContainer FailureTags;
	const TArray<uint8>& LocalBlockedAbilityBindings = AbilitySystem->GetBlockedAbilityBindings();

	if (AbilitySpec.IsActive())
	{
		OutStatusText = FString::Printf(TEXT(" (Active %d)"), AbilitySpec.ActiveCount);
		OutAbilityTextColor = FColor::Yellow;
	}
	else if (
		LocalBlockedAbilityBindings.IsValidIndex(AbilitySpec.InputID)
		&& LocalBlockedAbilityBindings[AbilitySpec.InputID])
	{
		OutStatusText = TEXT(" (InputBlocked)");
		OutAbilityTextColor = FColor::Red;
	}
	else if (Ability->GetAssetTags().HasAny(BlockedAbilityTags))
	{
		OutStatusText = TEXT(" (TagBlocked)");
		OutAbilityTextColor = FColor::Red;
	}
	else if (
		Ability->CanActivateAbility(
			AbilitySpec.Handle,
			AbilitySystem->AbilityActorInfo.Get(),
			nullptr,
			nullptr,
			OUT & FailureTags)
		== false)
	{
		OutStatusText = FString::Printf(TEXT("\n\t(CantActivate %s)"), *FailureTags.ToString());
		OutAbilityTextColor = FColor::Red;

		const float Cooldown = Ability->GetCooldownTimeRemaining(AbilitySystem->AbilityActorInfo.Get());
		if (Cooldown > 0.f)
		{
			OutStatusText += FString::Printf(TEXT("   Cooldown: %.2f\n"), Cooldown);
		}
	}
}

void FGameplayDebuggerCategory_OUUAbilities::DrawAbility(
	const FGameplayTagContainer& BlockedAbilityTags,
	const FGameplayAbilitySpec& AbilitySpec)
{
	if (AbilitySpec.Ability == nullptr)
		return;

	// #TODO-OUU Add debugging for instance-per-execution abilities. Right now only instance-per-actor and
	// non-instanced abilities are supported.
	// see AbilitySpec.GetAbilityInstances()

	const UGameplayAbility* Ability = AbilitySpec.GetPrimaryInstance();
	if (!IsValid(Ability))
	{
		Ability = AbilitySpec.Ability;
	}

	const FString AbilityName = OUU::Runtime::GameplayDebuggerUtils::CleanupName(GetNameSafe(AbilitySpec.Ability));
	if (!OUU::Runtime::RegexUtils::MatchesRegex(CVar_AbilityFilter.GetValueOnGameThread(), AbilityName))
		return;

	FString StatusText;
	FColor AbilityTextColor = FColorList::Grey;

	DetermineAbilityStatusText(BlockedAbilityTags, AbilitySpec, Ability, OUT StatusText, OUT AbilityTextColor);

	const FString InputPressedStr = AbilitySpec.InputPressed ? TEXT("(InputPressed)") : TEXT("");
	const FString ActivationModeStr = (AbilitySpec.IsActive() && AbilitySpec.Ability) ? UEnum::GetValueAsString(
										  TEXT("GameplayAbilities.EGameplayAbilityActivationMode"),
										  AbilitySpec.Ability->GetCurrentActivationInfoRef().ActivationMode)
																					  : TEXT("");

	Canvas->SetDrawColor(AbilityTextColor);

	const FString AbilitySourceName =
		OUU::Runtime::GameplayDebuggerUtils::CleanupName(GetNameSafe(AbilitySpec.SourceObject.Get()));

	DebugLine(
		FString::Printf(
			TEXT("%s (%s) %s %s %s"),
			*AbilityName,
			*AbilitySourceName,
			*StatusText,
			*InputPressedStr,
			*ActivationModeStr),
		4.f,
		0);

	if (!AbilitySpec.IsActive())
		return;

	TArray<UGameplayAbility*> Instances = AbilitySpec.GetAbilityInstances();
	for (int32 InstanceIdx = 0; InstanceIdx < Instances.Num(); ++InstanceIdx)
	{
		if (UOUUGameplayAbility* Instance = Cast<UOUUGameplayAbility>(Instances[InstanceIdx]))
		{
			DrawGameplayAbilityInstance(Instance);
			if (InstanceIdx < Instances.Num() - 2)
			{
				Canvas->SetDrawColor(FColorList::Grey);
				DebugLine(FString::Printf(TEXT("--------")), 7.f, 0);
			}
		}
	}
}

void FGameplayDebuggerCategory_OUUAbilities::DrawGameplayCue(
	UGameplayCueManager* CueManager,
	FString BaseCueTagString,
	UGameplayCueSet* CueSet,
	FGameplayTag ThisGameplayCueTag)
{
	FString CueTagString = ThisGameplayCueTag.ToString();
	CueTagString.RemoveFromStart(BaseCueTagString);
	int32 idx = CueSet->GameplayCueDataMap.FindChecked(ThisGameplayCueTag);
	if (idx == INDEX_NONE)
	{
		// ReSharper disable once CppUnreachableCode
		if (bPrintUnmappedCues)
		{
			Canvas->SetDrawColor(FColorList::Grey);
			DebugLine(FString::Printf(TEXT("%s -> unmapped"), *CueTagString), 0.f, 0);
		}
		return;
	}
	auto CueData = CueSet->GameplayCueData[idx];

	if (CueData.LoadedGameplayCueClass == nullptr)
	{
		if (bPrintNotLoadedCues)
		{
			Canvas->SetDrawColor(FColorList::Grey);
			DebugLine(FString::Printf(TEXT("%s -> not loaded"), *CueTagString), 0.f, 0);
		}
		return;
	}

	auto CueClass = CueData.LoadedGameplayCueClass;

	if (CueClass->GetDefaultObject<UGameplayCueNotify_Static>() != nullptr)
	{
		Canvas->SetDrawColor(FColorList::Grey);
		DebugLine(FString::Printf(TEXT("%s -> non-instanced"), *CueTagString), 0.f, 0);
	}
	else if (CueClass->GetDefaultObject<AGameplayCueNotify_Actor>() != nullptr)
	{
		Canvas->SetDrawColor(FColorList::White);

		DebugLine(FString::Printf(TEXT("%s -> actor"), *CueTagString), 0.f, 0);
	#if UE_VERSION_OLDER_THAN(5, 3, 0)
		AActor* LocalAvatarActor = AbilitySystem->GetAvatarActor_Direct();
		AActor* LocalOwnerActor = AbilitySystem->GetOwnerActor();
		for (auto CueEntry : CueManager->NotifyMapActor)
		{
			FGCNotifyActorKey Key = CueEntry.Key;
			if (Key.CueClass != CueClass)
				continue;

			AGameplayCueNotify_Actor* CueActor = CueEntry.Value.Get();
			bool bIsValidForThisACS =
				(Key.TargetActor == LocalAvatarActor || Key.TargetActor == LocalOwnerActor) && IsValid(CueActor);

			Canvas->SetDrawColor(bIsValidForThisACS ? FColorList::Green : FColorList::Grey);

			DebugLine(OUU::Runtime::GameplayDebuggerUtils::CleanupName(CueClass->GetName()), 7.f, 0);
		}
	#else
		DebugLine(TEXT("no NotifyMapActor since UE 5.3.0"), 7.f, 0);
	#endif
	}
	else
	{
		Canvas->SetDrawColor(FColorList::Red);
		DebugLine(
			FString::Printf(TEXT("%s -> unsupported cue class %s"), *CueTagString, *GetNameSafe(CueClass)),
			0.f,
			0);
	}
}

void FGameplayDebuggerCategory_OUUAbilities::DrawAttribute(FGameplayAttribute& Attribute, bool ColorSwitch)
{
	FGameplayEffectAttributeCaptureDefinition
		CaptureDef(Attribute.GetUProperty(), EGameplayEffectAttributeCaptureSource::Source, false);
	FGameplayEffectAttributeCaptureSpec CaptureSpec(CaptureDef);
	AbilitySystem->CaptureAttributeForGameplayEffect(CaptureSpec);

	// Source Tags
	static FGameplayTagContainer QuerySourceTags;
	QuerySourceTags.Reset();

	AbilitySystem->GetOwnedGameplayTags(QuerySourceTags);
	// QuerySourceTags.AppendTags(_SourceTags);

	// Target Tags
	static FGameplayTagContainer QueryTargetTags;
	QueryTargetTags.Reset();

	// QueryTargetTags.AppendTags(_TargetTags);

	// Define parameters
	FAggregatorEvaluateParameters Params;
	Params.SourceTags = &QuerySourceTags;
	Params.TargetTags = &QueryTargetTags;
	Params.IncludePredictiveMods = true;

	float BaseValue = AbilitySystem->GetNumericAttributeBase(Attribute);
	float QualifiedValue = AbilitySystem->GetNumericAttribute(Attribute);

	FString PaddedAttributeName = Attribute.GetName();
	while (PaddedAttributeName.Len() < 30)
		PaddedAttributeName += " ";

	FString AttributeString =
		FString::Printf(TEXT("%s %.2f (Base: %.2f)"), *PaddedAttributeName, QualifiedValue, BaseValue);

	Canvas->SetDrawColor(ColorSwitch ? FColor::White : FColor::Emerald);
	DebugLine(AttributeString, 4.f, 0);
}

void FGameplayDebuggerCategory_OUUAbilities::DrawDebugBody()
{
	FGameplayTagContainer BlockedAbilityTags;
	AbilitySystem->GetBlockedAbilityTags(OUT BlockedAbilityTags);

	#define DEBUG_BODY_SECTION(Title)                                                                                  \
		DrawTitle(Title);                                                                                              \
		ON_SCOPE_EXIT                                                                                                  \
		{                                                                                                              \
			Canvas->SetDrawColor(FColor::White);                                                                       \
			AccumulateScreenPos();                                                                                     \
			NewColumnForCategory_Optional();                                                                           \
		};

	// First the categories that have a pretty stable length - then the categories that are more fluctuating.
	// That way the entries don't jump around AS MUCH on the screen.

	{
		DEBUG_BODY_SECTION("ATTRIBUTES")

		TArray<FGameplayAttribute> AllAttributes;
		AbilitySystem->GetAllAttributes(OUT AllAttributes);
		bool ColorSwitch = true;
		for (auto& Attribute : AllAttributes)
		{
			DrawAttribute(Attribute, ColorSwitch);
			ColorSwitch = !ColorSwitch;
		}
	}

	NewColumn();

	{
		DEBUG_BODY_SECTION("ABILITIES")

		TArray<FGameplayAbilitySpec> AbilitiesSortedByName = AbilitySystem->GetActivatableAbilities();
		AbilitiesSortedByName.Sort([](const FGameplayAbilitySpec& A, const FGameplayAbilitySpec& B) -> bool {
			if (!IsValid(A.Ability) || !IsValid(B.Ability))
				return false;

			return A.Ability->GetName() < B.Ability->GetName();
		});

		for (const FGameplayAbilitySpec& AbilitySpec : AbilitiesSortedByName)
		{
			DrawAbility(BlockedAbilityTags, AbilitySpec);
		}
	}

	{
		DEBUG_BODY_SECTION("CUES")
		UGameplayCueManager* CueManager = UAbilitySystemGlobals::Get().GetGameplayCueManager();
		auto BaseCueTag = UGameplayCueSet::BaseGameplayCueTag();
		FString BaseCueTagString = BaseCueTag.ToString() + TEXT(".");
		FGameplayTagContainer AllGameplayCueTags = UGameplayTagsManager::Get().RequestGameplayTagChildren(BaseCueTag);
		auto CueSet = CueManager->GetRuntimeCueSet();
		for (FGameplayTag ThisGameplayCueTag : AllGameplayCueTags)
		{
			DrawGameplayCue(CueManager, BaseCueTagString, CueSet, ThisGameplayCueTag);
		}
	}

	{
		DEBUG_BODY_SECTION("TAGS")
		FGameplayTagContainer OwnerTags;
		AbilitySystem->GetOwnedGameplayTags(OwnerTags);
		AddTagList(OwnerTags, "OwnedTags");
		AddTagList(BlockedAbilityTags, "BlockedAbilityTags");
	}

	NewColumn();

	{
		DEBUG_BODY_SECTION("GAMEPLAY EFFECTS")
		for (FActiveGameplayEffect& ActiveGE : &(AbilitySystem->ActiveGameplayEffects))
		{
			DrawGameplayEffect(ActiveGE);
		}
	}

	{
		DEBUG_BODY_SECTION("GAMEPLAY EVENTS")
		for (auto Entry : ReverseRange(AbilitySystem->CircularGameplayEventHistory))
		{
			DebugLine(
				FString::Printf(
					TEXT("#%2i: [%s](%.2f)"),
					Entry.EventNumber,
					*Entry.EventTag.ToString(),
					Entry.EventMagnitude),
				0.f,
				0);
			/*
			DebugLine(
				FString::Printf(
					TEXT("instigator: %s | %s"),
					*LexToString(Entry.Instigator),
					*Entry.InstigatorTags.ToStringSimple()),
				10.f,
				0);
			DebugLine(
				FString::Printf(
					TEXT("target: %s | %s"),
					*LexToString(Entry.Target),
					*Entry.TargetTags.ToStringSimple()),
				10.f,
				0);
			*/
			// since the tag containers in the even buffer cause crashes, here's a version without them:
			DebugLine(
				FString::Printf(TEXT("%s -> %s"), *LexToString(Entry.Instigator), *LexToString(Entry.Target)),
				10.f,
				0);
		}
	}
}

void FGameplayDebuggerCategory_OUUAbilities::DrawTitle(const FString& DebugTitle)
{
	Canvas->SetDrawColor(FColor::White);
	FFontRenderInfo RenderInfo = FFontRenderInfo();
	RenderInfo.bEnableShadow = true;
	constexpr float HeadingScale = 1.0f;
	const auto LargeFont = GEngine->GetLargeFont();
	DebugInfo.LineHeight =
		Canvas->DrawText(LargeFont, DebugTitle, DebugInfo.XPos, DebugInfo.YPos, HeadingScale, HeadingScale, RenderInfo);
	DebugInfo.LineHeight += 5.f;
	DebugInfo.LineHeight = FMath::Max(DebugInfo.LineHeight, LargeFont->GetMaxCharHeight() * HeadingScale);
	AccumulateScreenPos();
}

void FGameplayDebuggerCategory_OUUAbilities::DrawDebugHeader()
{
	FString DebugTitle("");
	const AActor* LocalAvatarActor = AbilitySystem->GetAvatarActor_Direct();
	const AActor* LocalOwnerActor = AbilitySystem->GetOwnerActor();

	// Avatar info
	if (LocalAvatarActor)
	{
		const ENetRole AvatarRole = LocalAvatarActor->GetLocalRole();
		DebugTitle += FString::Printf(TEXT("avatar %s "), *LocalAvatarActor->GetName());
		if (AvatarRole == ROLE_AutonomousProxy)
		{
			DebugTitle += TEXT("(local player) ");
		}
		else if (AvatarRole == ROLE_SimulatedProxy)
		{
			DebugTitle += TEXT("(simulated) ");
		}
		else if (AvatarRole == ROLE_Authority)
		{
			DebugTitle += TEXT("(authority) ");
		}
	}

	// Owner info
	if (LocalOwnerActor && LocalOwnerActor != LocalAvatarActor)
	{
		const ENetRole OwnerRole = LocalOwnerActor->GetLocalRole();
		DebugTitle += FString::Printf(TEXT("for owner %s "), *LocalOwnerActor->GetName());
		if (OwnerRole == ROLE_AutonomousProxy)
		{
			DebugTitle += TEXT("(autonomous) ");
		}
		else if (OwnerRole == ROLE_SimulatedProxy)
		{
			DebugTitle += TEXT("(simulated) ");
		}
		else if (OwnerRole == ROLE_Authority)
		{
			DebugTitle += TEXT("(authority) ");
		}
	}

	DrawTitle(DebugTitle);
	DrawTitle("");
}

void FGameplayDebuggerCategory_OUUAbilities::AccumulateScreenPos()
{
	const float NewY = DebugInfo.YPos + DebugInfo.LineHeight;
	if (NewY > DebugInfo.MaxY)
	{
		NewColumn();
	}
	else
	{
		DebugInfo.YPos = NewY;
	}
}

void FGameplayDebuggerCategory_OUUAbilities::NewColumn()
{
	DebugInfo.YPos = DebugInfo.MinY;
	const float MaxX = Canvas->ClipX;
	const float ColumnWidth = MaxX * (1.f / CVar_NumColumns.GetValueOnGameThread());
	DebugInfo.XPos += ColumnWidth;
	if (DebugInfo.XPos > MaxX)
	{
		Canvas->SetDrawColor(FColor::Yellow);
		FFontRenderInfo RenderInfo = FFontRenderInfo();
		RenderInfo.bEnableShadow = true;
		const UFont* LargeFont = GEngine->GetLargeFont();
		Canvas->DrawText(
			LargeFont,
			TEXT("COLUMN OVERSPILL"),
			DebugInfo.XPos + 4.f,
			DebugInfo.YPos - LargeFont->GetMaxCharHeight(),
			1.f,
			1.f,
			RenderInfo);
	}
}

void FGameplayDebuggerCategory_OUUAbilities::NewColumnForCategory_Optional()
{
	if (DebugInfo.YPos > DebugInfo.MinY + (DebugInfo.MaxY - DebugInfo.MinY) / 0.6f)
	{
		NewColumn();
	}
}

void FGameplayDebuggerCategory_OUUAbilities::DebugLine(const FString& Str, float XOffset, int32 MinTextRowsToAdvance)
{
	FFontRenderInfo RenderInfo = FFontRenderInfo();
	RenderInfo.bEnableShadow = true;
	if (const UFont* Font = GEngine->GetTinyFont())
	{
		DebugInfo.LineHeight =
			Canvas->DrawText(Font, Str, DebugInfo.XPos + XOffset, DebugInfo.YPos, 1.f, 1.f, RenderInfo);
		DebugInfo.LineHeight = FMath::Max(DebugInfo.LineHeight, MinTextRowsToAdvance * Font->GetMaxCharHeight());
		AccumulateScreenPos();
	}
}

void FGameplayDebuggerCategory_OUUAbilities::AddTagList(FGameplayTagContainer Tags, const FString& TagsListTitle)
{
	int32 TagCount = 1;
	const int32 NumTags = Tags.Num();
	FString CombinedTagsString = "";
	for (FGameplayTag Tag : Tags)
	{
		CombinedTagsString.Append(FString::Printf(TEXT("\n%s (%d)"), *Tag.ToString(), AbilitySystem->GetTagCount(Tag)));

		if (TagCount++ < NumTags)
		{
			CombinedTagsString += TEXT(", ");
		}
	}

	DebugLine(FString::Printf(TEXT("%s: %s"), *TagsListTitle, *CombinedTagsString), 4.f, 2);
	DebugLine("", 0.f, 2);
}

#endif
