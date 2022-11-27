// Copyright (c) 2022 Jonas Reich

#include "GameplayAbilities/Debug/GameplayDebuggerCategory_OUUAbilities.h"

#include "TimerManager.h"

#if WITH_GAMEPLAY_DEBUGGER

	#include "AbilitySystemComponent.h"
	#include "AbilitySystemGlobals.h"
	#include "AbilitySystemLog.h"
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

TAutoConsoleVariable<FString> AbilityFilter{
	TEXT("ouu.Debug.Ability.Filter"),
	TEXT(".*"),
	TEXT("Regular expression filter for ability names. Default value: '.*' (allow all)."),
	ECVF_Cheat};

void FGameplayDebuggerCategory_OUUAbilities::DrawBackground(
	FGameplayDebuggerCanvasContext& CanvasContext,
	const FVector2D& BackgroundLocation,
	const FVector2D& BackgroundSize)
{
	// Draw a transparent background so that the text is easier to look at
	const FLinearColor BackgroundColor(0.1f, 0.1f, 0.1f, 0.8f);
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

	if (auto* AbilityComp = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(FindLocalDebugActor()))
	{
		if (auto* OUUAbilityComp = Cast<UOUUAbilitySystemComponent>(AbilityComp))
		{
			FVector2D ViewPortSize;
			GEngine->GameViewport->GetViewportSize(OUT ViewPortSize);
			const float RemainingViewportHeight = ViewPortSize.Y - CanvasContext.CursorY;

			constexpr float BackgroundPadding = 5.0f;
			FAbilitySystemComponentDebugInfo DebugInfo;
			DebugInfo.bPrintToLog = false;

			DebugInfo.Canvas = CanvasContext.Canvas.Get();
			DebugInfo.XPos = CanvasContext.CursorX;
			DebugInfo.YPos = CanvasContext.CursorY;
			DebugInfo.OriginalX = CanvasContext.CursorX;
			DebugInfo.OriginalY = CanvasContext.CursorY;
			// Add some extra padding on the bottom.
			// It's likely the task bar overlaps or some other clipping issues cause content to get lost here otherwise.
			DebugInfo.MaxY = ViewPortSize.Y - BackgroundPadding - 50.f;
			DebugInfo.NewColumnYPadding = DebugInfo.OriginalY + 30.f;

			const FVector2D BackgroundLocation =
				FVector2D(BackgroundPadding, CanvasContext.CursorY - BackgroundPadding);
			const FVector2D BackgroundSize(
				ViewPortSize.X - 2 * BackgroundPadding,
				RemainingViewportHeight - 2 * BackgroundPadding);

			DrawBackground(CanvasContext, BackgroundLocation, BackgroundSize);

			// Use custom debugger for UOUUAbilitySystemComponents
			Debug_Custom(DebugInfo, OUUAbilityComp);
			CanvasContext.CursorY = ViewPortSize.Y;
		}
		else
		{
			CanvasContext.Printf(
				TEXT("{yellow}Ability system component %s (%s) is not a UOUUAbilitySystemComponent.\nPlease "
					 "use the regular ability system debugger or reparent your component class."),
				*AbilityComp->GetName(),
				*AbilityComp->GetClass()->GetName());
		}
	}
}

void FGameplayDebuggerCategory_OUUAbilities::Debug_Custom(
	FAbilitySystemComponentDebugInfo& Info,
	UOUUAbilitySystemComponent* AbilitySystem)
{
	DrawDebugHeader(Info, AbilitySystem);

	FGameplayTagContainer BlockedAbilityTags;
	AbilitySystem->GetBlockedAbilityTags(OUT BlockedAbilityTags);

	{
		DrawTitle(Info, "TAGS");

		FGameplayTagContainer OwnerTags;
		AbilitySystem->GetOwnedGameplayTags(OwnerTags);

		if (Info.Canvas)
		{
			Info.Canvas->SetDrawColor(FColor::White);
		}

		AddTagList(Info, AbilitySystem, OwnerTags, "OwnedTags");
		AddTagList(Info, AbilitySystem, BlockedAbilityTags, "BlockedAbilityTags");

		if (Info.YPos > Info.NewColumnYPadding + (Info.MaxY - Info.NewColumnYPadding) / 0.6f)
		{
			NewColumn(Info);
		}
	}

	TSet<FGameplayAttribute> DrawAttributes;

	float MaxCharHeight = 10;
	if (AbilitySystem->GetOwner()->GetNetMode() != NM_DedicatedServer)
	{
		MaxCharHeight = GEngine->GetTinyFont()->GetMaxCharHeight();
	}

	// -------------------------------------------------------------

	{
		DrawTitle(Info, "GAMEPLAY EFFECTS");

		for (FActiveGameplayEffect& ActiveGE : &(AbilitySystem->ActiveGameplayEffects))
		{
			if (Info.Canvas)
			{
				Info.Canvas->SetDrawColor(FColor::White);
			}

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
			if (ActiveGE.Spec.StackCount > 1)
			{
				if (ActiveGE.Spec.Def->StackingType == EGameplayEffectStackingType::AggregateBySource)
				{
					StackString = FString::Printf(
						TEXT("(Stacks: %d. From: %s) "),
						ActiveGE.Spec.StackCount,
						*GetNameSafe(
							ActiveGE.Spec.GetContext().GetInstigatorAbilitySystemComponent()->GetAvatarActor_Direct()));
				}
				else
				{
					StackString = FString::Printf(TEXT("(Stacks: %d) "), ActiveGE.Spec.StackCount);
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
				if (ActiveGE.PredictionKey.WasLocallyGenerated())
				{
					PredictionString = FString::Printf(TEXT("(Predicted and Waiting)"));
				}
				else
				{
					PredictionString = FString::Printf(TEXT("(Predicted and Caught Up)"));
				}
			}

			if (Info.Canvas)
			{
				Info.Canvas->SetDrawColor(ActiveGE.bIsInhibited ? FColorList::Grey : FColor::White);
			}

			DebugLine(
				Info,
				FString::Printf(
					TEXT("%s %s %s %s %s"),
					*OUU::Runtime::GameplayDebuggerUtils::CleanupName(GetNameSafe(ActiveGE.Spec.Def)),
					*DurationStr,
					*StackString,
					*LevelString,
					*PredictionString),
				4.f,
				0.f);

			FGameplayTagContainer GrantedTags;
			ActiveGE.Spec.GetAllGrantedTags(GrantedTags);
			if (GrantedTags.Num() > 0)
			{
				DebugLine(Info, FString::Printf(TEXT("Granted Tags: %s"), *GrantedTags.ToStringSimple()), 7.f, 0.f);
			}

			for (int32 ModIdx = 0; ModIdx < ActiveGE.Spec.Modifiers.Num(); ++ModIdx)
			{
				if (ActiveGE.Spec.Def == nullptr)
				{
					DebugLine(Info, FString::Printf(TEXT("null def! (Backwards compat?)")), 7.f, 0.f);
					continue;
				}

				const FModifierSpec& ModSpec = ActiveGE.Spec.Modifiers[ModIdx];
				const FGameplayModifierInfo& ModInfo = ActiveGE.Spec.Def->Modifiers[ModIdx];

				DebugLine(
					Info,
					FString::Printf(
						TEXT("Mod: %s. %s. %.2f"),
						*ModInfo.Attribute.GetName(),
						*EGameplayModOpToString(ModInfo.ModifierOp),
						ModSpec.GetEvaluatedMagnitude()),
					7.f,
					0.f);

				if (Info.Canvas)
				{
					Info.Canvas->SetDrawColor(ActiveGE.bIsInhibited ? FColor(128, 128, 128) : FColor::White);
				}
			}

			AccumulateScreenPos(Info);
		}

		NewColumnForCategory_Optional(Info);
	}

	// -------------------------------------------------------------

	{
		DrawTitle(Info, "ATTRIBUTES");

		TArray<FGameplayAttribute> AllAtributes;
		AbilitySystem->GetAllAttributes(AllAtributes);
		for (auto& Attribute : AllAtributes)
		{
			FAggregator SnaphotAggregator;
			GetAttributeAggregatorSnapshot(AbilitySystem, Attribute, OUT SnaphotAggregator);

			FAggregatorEvaluateParameters EmptyParams;
			SnaphotAggregator.EvaluateQualificationForAllMods(EmptyParams);
			TMap<EGameplayModEvaluationChannel, const TArray<FAggregatorMod>*> ModMap;
			SnaphotAggregator.GetAllAggregatorMods(ModMap);

			if (ModMap.Num() == 0)
			{
				continue;
			}

			float FinalValue = AbilitySystem->GetNumericAttribute(Attribute);
			float BaseValue = SnaphotAggregator.GetBaseValue();

			FString PaddedAttributeName = Attribute.GetName();
			while (PaddedAttributeName.Len() < 30)
				PaddedAttributeName += " ";

			FString AttributeString =
				FString::Printf(TEXT("%s %.2f "), *PaddedAttributeName, AbilitySystem->GetNumericAttribute(Attribute));
			if (FMath::Abs<float>(BaseValue - FinalValue) > SMALL_NUMBER)
			{
				AttributeString += FString::Printf(TEXT(" (Base: %.2f)"), BaseValue);
			}

			if (Info.Canvas)
			{
				Info.Canvas->SetDrawColor(FColor::White);
			}

			DebugLine(Info, AttributeString, 4.f, 0.f);

			DrawAttributes.Add(Attribute);

			for (const auto& CurMapElement : ModMap)
			{
				const EGameplayModEvaluationChannel Channel = CurMapElement.Key;
				const TArray<FAggregatorMod>* ModArrays = CurMapElement.Value;

				const FString ChannelNameString =
					UAbilitySystemGlobals::Get().GetGameplayModEvaluationChannelAlias(Channel).ToString();
				for (int32 ModOpIdx = 0; ModOpIdx < EGameplayModOp::Max; ++ModOpIdx)
				{
					const TArray<FAggregatorMod>& CurModArray = ModArrays[ModOpIdx];
					for (const FAggregatorMod& Mod : CurModArray)
					{
						bool IsActivelyModifyingAttribute = Mod.Qualifies();
						if (Info.Canvas)
						{
							Info.Canvas->SetDrawColor(
								IsActivelyModifyingAttribute ? FColor::Yellow : FColor(128, 128, 128));
						}

						FActiveGameplayEffect* ActiveGE =
							AbilitySystem->ActiveGameplayEffects.GetActiveGameplayEffect(Mod.ActiveHandle);
						FString SrcName = ActiveGE ? ActiveGE->Spec.Def->GetName() : FString(TEXT(""));

						if (IsActivelyModifyingAttribute == false)
						{
							if (Mod.SourceTagReqs)
							{
								SrcName += FString::Printf(TEXT(" SourceTags: [%s] "), *Mod.SourceTagReqs->ToString());
							}
							if (Mod.TargetTagReqs)
							{
								SrcName += FString::Printf(TEXT("TargetTags: [%s]"), *Mod.TargetTagReqs->ToString());
							}
						}

						DebugLine(
							Info,
							FString::Printf(
								TEXT("   %s %s\t %.2f - %s"),
								*ChannelNameString,
								*EGameplayModOpToString(ModOpIdx),
								Mod.EvaluatedMagnitude,
								*SrcName),
							7.f,
							0.f);
						Info.NewColumnYPadding = FMath::Max<float>(Info.NewColumnYPadding, Info.YPos + Info.YL);
					}
				}
			}

			AccumulateScreenPos(Info);
		}

		if (Info.Canvas)
		{
			Info.Canvas->SetDrawColor(FColor::White);
		}
		for (UAttributeSet* Set : AbilitySystem->GetSpawnedAttributes())
		{
			if (!Set)
			{
				continue;
			}

			for (TFieldIterator<FProperty> It(Set->GetClass()); It; ++It)
			{
				FGameplayAttribute Attribute(*It);

				if (DrawAttributes.Contains(Attribute))
					continue;

				if (Attribute.IsValid())
				{
					float Value = AbilitySystem->GetNumericAttribute(Attribute);
					FString PaddedAttributeName = Attribute.GetName();
					while (PaddedAttributeName.Len() < 30)
						PaddedAttributeName += " ";

					DebugLine(Info, FString::Printf(TEXT("%s %.2f"), *PaddedAttributeName, Value), 4.f, 0.f);
				}
			}
		}
		AccumulateScreenPos(Info);

		NewColumnForCategory_Optional(Info);
	}

	// -------------------------------------------------------------

	{
		DrawTitle(Info, "ABILITIES");

		TArray<FGameplayAbilitySpec> ActivatableAbilities = AbilitySystem->GetActivatableAbilities();

		// Sort abilities by name
		ActivatableAbilities.Sort([](const FGameplayAbilitySpec& A, const FGameplayAbilitySpec& B) -> bool {
			if (!IsValid(A.Ability) || !IsValid(B.Ability))
				return false;

			return A.Ability->GetName() < B.Ability->GetName();
		});

		for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities)
		{
			if (AbilitySpec.Ability == nullptr)
				continue;

			// #TODO-OUU Add debugging for instance-per-execution abilities. Right now only instance-per-actor and
			// non-instanced abilities are supported.
			UGameplayAbility* Ability = AbilitySpec.GetPrimaryInstance();
			if (!IsValid(Ability))
			{
				Ability = AbilitySpec.Ability;
			}

			const FString AbilityName =
				OUU::Runtime::GameplayDebuggerUtils::CleanupName(GetNameSafe(AbilitySpec.Ability));
			if (!OUU::Runtime::RegexUtils::MatchesRegex(AbilityFilter.GetValueOnGameThread(), AbilityName))
				continue;

			FString StatusText;
			FColor AbilityTextColor = FColorList::Grey;
			FGameplayTagContainer FailureTags;

			const TArray<uint8>& LocalBlockedAbilityBindings = AbilitySystem->GetBlockedAbilityBindings();

			if (AbilitySpec.IsActive())
			{
				StatusText = FString::Printf(TEXT(" (Active %d)"), AbilitySpec.ActiveCount);
				AbilityTextColor = FColor::Yellow;
			}
			else if (
				LocalBlockedAbilityBindings.IsValidIndex(AbilitySpec.InputID)
				&& LocalBlockedAbilityBindings[AbilitySpec.InputID])
			{
				StatusText = TEXT(" (InputBlocked)");
				AbilityTextColor = FColor::Red;
			}
			else if (Ability->AbilityTags.HasAny(BlockedAbilityTags))
			{
				StatusText = TEXT(" (TagBlocked)");
				AbilityTextColor = FColor::Red;
			}
			else if (
				Ability->CanActivateAbility(
					AbilitySpec.Handle,
					AbilitySystem->AbilityActorInfo.Get(),
					nullptr,
					nullptr,
					&FailureTags)
				== false)
			{
				StatusText = FString::Printf(TEXT(" (CantActivate %s)"), *FailureTags.ToString());
				AbilityTextColor = FColor::Red;

				float Cooldown = Ability->GetCooldownTimeRemaining(AbilitySystem->AbilityActorInfo.Get());
				if (Cooldown > 0.f)
				{
					StatusText += FString::Printf(TEXT("   Cooldown: %.2f\n"), Cooldown);
				}
			}

			FString InputPressedStr = AbilitySpec.InputPressed ? TEXT("(InputPressed)") : TEXT("");
			FString ActivationModeStr = AbilitySpec.IsActive() ? UEnum::GetValueAsString(
											TEXT("GameplayAbilities.EGameplayAbilityActivationMode"),
											AbilitySpec.ActivationInfo.ActivationMode)
															   : TEXT("");

			if (Info.Canvas)
			{
				Info.Canvas->SetDrawColor(AbilityTextColor);
			}

			const FString AbilitySourceName =
				OUU::Runtime::GameplayDebuggerUtils::CleanupName(GetNameSafe(AbilitySpec.SourceObject.Get()));

			DebugLine(
				Info,
				FString::Printf(
					TEXT("%s (%s) %s %s %s"),
					*AbilityName,
					*AbilitySourceName,
					*StatusText,
					*InputPressedStr,
					*ActivationModeStr),
				4.f,
				0.f);

			if (AbilitySpec.IsActive())
			{
				TArray<UGameplayAbility*> Instances = AbilitySpec.GetAbilityInstances();
				for (int32 InstanceIdx = 0; InstanceIdx < Instances.Num(); ++InstanceIdx)
				{
					UOUUGameplayAbility* Instance = Cast<UOUUGameplayAbility>(Instances[InstanceIdx]);
					if (!Instance)
						continue;

					// #TODO-OUU Add error/warning/fallback for non ouu abilities

					if (Info.Canvas)
					{
						Info.Canvas->SetDrawColor(FColor::White);
					}
					for (UGameplayTask* Task : Instance->ActiveTasks)
					{
						if (Task)
						{
							DebugLine(Info, FString::Printf(TEXT("%s"), *Task->GetDebugString()), 7.f, 0.f);

							for (FAbilityTaskDebugMessage& Msg : Instance->TaskDebugMessages)
							{
								if (Msg.FromTask == Task)
								{
									DebugLine(Info, FString::Printf(TEXT("%s"), *Msg.Message), 9.f, 0.f);
								}
							}
						}
					}

					bool FirstTaskMsg = true;
					int32 MsgCount = 0;
					for (FAbilityTaskDebugMessage& Msg : ReverseRange(Instance->TaskDebugMessages))
					{
						if (Instance->ActiveTasks.Contains(Msg.FromTask) == false)
						{
							// Cap finished task msgs to 5 per ability if we are printing to screen (else things will
							// scroll off)
							if (Info.Canvas && ++MsgCount > 5)
							{
								break;
							}

							if (FirstTaskMsg)
							{
								DebugLine(
									Info,
									FString::Printf(
										TEXT("[FinishedTasks (last x of %i)]"),
										Instance->TaskDebugMessages.Num()),
									7.f,
									0.f);
								FirstTaskMsg = false;
							}

							DebugLine(Info, FString::Printf(TEXT("%s"), *Msg.Message), 9.f, 0.f);
						}
					}

					if (InstanceIdx < Instances.Num() - 2)
					{
						if (Info.Canvas)
						{
							Info.Canvas->SetDrawColor(FColorList::Grey);
						}
						DebugLine(Info, FString::Printf(TEXT("--------")), 7.f, 0.f);
					}
				}
			}
		}
		AccumulateScreenPos(Info);
		NewColumnForCategory_Optional(Info);
	}

	// -------------------------------------------------------------

	{
		DrawTitle(Info, "CUES");

		const bool bPrintNotLoadedCues = false;
		const bool bPrintUnmappedCues = false;

		UGameplayCueManager* CueManager = UAbilitySystemGlobals::Get().GetGameplayCueManager();
		auto BaseCueTag = UGameplayCueSet::BaseGameplayCueTag();
		FString BaseCueTagString = BaseCueTag.ToString();
		FGameplayTagContainer AllGameplayCueTags = UGameplayTagsManager::Get().RequestGameplayTagChildren(BaseCueTag);
		auto CueSet = CueManager->GetRuntimeCueSet();
		for (FGameplayTag ThisGameplayCueTag : AllGameplayCueTags)
		{
			FString CueTagString = ThisGameplayCueTag.ToString();
			CueTagString.RemoveFromStart(BaseCueTagString);
			int32 idx = CueSet->GameplayCueDataMap.FindChecked(ThisGameplayCueTag);
			if (idx != INDEX_NONE)
			{
				auto CueData = CueSet->GameplayCueData[idx];

				if (CueData.LoadedGameplayCueClass == nullptr)
				{
					if (bPrintNotLoadedCues)
					{
						if (Info.Canvas)
						{
							Info.Canvas->SetDrawColor(FColorList::Grey);
						}
						DebugLine(Info, FString::Printf(TEXT("%s -> not loaded"), *CueTagString), 0.f, 0.f);
					}
					continue;
				}

				auto CueClass = CueData.LoadedGameplayCueClass;

				if (UGameplayCueNotify_Static* NonInstancedCueCDO =
						Cast<UGameplayCueNotify_Static>(CueClass->ClassDefaultObject))
				{
					if (Info.Canvas)
					{
						Info.Canvas->SetDrawColor(FColorList::Grey);
					}
					DebugLine(Info, FString::Printf(TEXT("%s -> non-instanced"), *CueTagString), 0.f, 0.f);
				}
				else if (
					AGameplayCueNotify_Actor* InstancedCueCDO =
						Cast<AGameplayCueNotify_Actor>(CueClass->ClassDefaultObject))
				{
					AActor* LocalAvatarActor = AbilitySystem->GetAvatarActor_Direct();
					AActor* LocalOwnerActor = AbilitySystem->GetOwnerActor();
					if (Info.Canvas)
					{
						Info.Canvas->SetDrawColor(FColorList::White);
					}
					DebugLine(Info, FString::Printf(TEXT("%s -> actor"), *CueTagString), 0.f, 0.f);
					for (auto CueEntry : CueManager->NotifyMapActor)
					{
						FGCNotifyActorKey Key = CueEntry.Key;
						if (Key.CueClass != CueClass)
							continue;

						AGameplayCueNotify_Actor* CueActor = CueEntry.Value.Get();
						bool bIsValidForThisACS =
							(Key.TargetActor == LocalAvatarActor || Key.TargetActor == LocalOwnerActor)
							&& IsValid(CueActor);
						if (Info.Canvas)
						{
							Info.Canvas->SetDrawColor(bIsValidForThisACS ? FColorList::Green : FColorList::Grey);
						}
						DebugLine(
							Info,
							OUU::Runtime::GameplayDebuggerUtils::CleanupName(CueClass->GetName()),
							7.f,
							0.f);
					}
				}
			}
			else if (bPrintUnmappedCues)
			{
				if (Info.Canvas)
				{
					Info.Canvas->SetDrawColor(FColorList::Grey);
				}
				DebugLine(Info, FString::Printf(TEXT("%s -> unmapped"), *CueTagString), 0.f, 0.f);
			}
		}

		AccumulateScreenPos(Info);
		NewColumnForCategory_Optional(Info);
	}

	// -------------------------------------------------------------

	{
		DrawTitle(Info, "GAMEPLAY EVENTS");

		auto Now = FDateTime::Now();
		for (auto Entry : ReverseRange(AbilitySystem->CircularGameplayEventHistory))
		{
			float SecondsSinceEvent = (Now - Entry.Timestamp).GetTotalSeconds();
			FNumberFormattingOptions NumberFormattingOptions;
			NumberFormattingOptions.MinimumIntegralDigits = 3;
			NumberFormattingOptions.MaximumIntegralDigits = 3;
			NumberFormattingOptions.MinimumFractionalDigits = 2;
			NumberFormattingOptions.MaximumFractionalDigits = 2;
			FString SecondsSinceEventString = FText::AsNumber(SecondsSinceEvent, &NumberFormattingOptions).ToString();

			FString DebugString = FString::Printf(
				TEXT("#%2i: [%s](%.2f) %s -> %s"),
				Entry.EventNumber,
				*Entry.EventTag.ToString(),
				Entry.EventMagnitude,
				*LexToString(Entry.Instigator),
				*LexToString(Entry.Target));
			DebugLine(Info, DebugString, 0.f, 0.f);
		}

		AccumulateScreenPos(Info);
		NewColumnForCategory_Optional(Info);
	}

	if (Info.XPos > Info.OriginalX)
	{
		// We flooded to new columns, returned YPos should be max Y (and some padding)
		Info.YPos = Info.MaxY + MaxCharHeight * 2.f;
	}
	Info.YL = MaxCharHeight;
}

void FGameplayDebuggerCategory_OUUAbilities::GetAttributeAggregatorSnapshot(
	UOUUAbilitySystemComponent* AbilitySystem,
	FGameplayAttribute& Attribute,
	FAggregator SnaphotAggregator)
{
	// NOTE: As of writing this code, this is how I understand the usage of CaptureSource and bSnapshot.
	//       There might be some misunderstandings, so feel free to make corrections and add an appropriate explanation,
	//       if you know better!

	// We pick target, because we pretend that we build this capture for an effect that modifies the attributes on this
	// ASC.
	const EGameplayEffectAttributeCaptureSource CaptureSource = EGameplayEffectAttributeCaptureSource::Target;
	// We pick snapshot, because we do not want any future updates of the values, just a single snapshot.
	const bool bSnapshot = true;

	FGameplayEffectAttributeCaptureDefinition CaptureDefinition{Attribute, CaptureSource, bSnapshot};
	FGameplayEffectAttributeCaptureSpec CaptureSpec{CaptureDefinition};
	AbilitySystem->CaptureAttributeForGameplayEffect(IN OUT CaptureSpec);

	bool bGotSnapshot = CaptureSpec.AttemptGetAttributeAggregatorSnapshot(OUT SnaphotAggregator);
	ensureAlwaysMsgf(
		bGotSnapshot,
		TEXT("Snapshots should always be successful! "
			 "The spec is captured immediately before and should only fail if it's uncaptured. "
			 "See docs of FGameplayEffectAttributeCaptureSpec::AttemptGetAttributeAggregatorSnapshot."));
}

void FGameplayDebuggerCategory_OUUAbilities::DrawTitle(FAbilitySystemComponentDebugInfo& Info, FString DebugTitle)
{
	if (Info.Canvas)
	{
		Info.Canvas->SetDrawColor(FColor::White);
		FFontRenderInfo RenderInfo = FFontRenderInfo();
		RenderInfo.bEnableShadow = true;
		const float HeadingScale = 1.0f;
		auto LargeFont = GEngine->GetLargeFont();
		Info.YL =
			Info.Canvas->DrawText(LargeFont, DebugTitle, Info.XPos, Info.YPos, HeadingScale, HeadingScale, RenderInfo);
		Info.YL = FMath::Max(Info.YL, LargeFont->GetMaxCharHeight() * HeadingScale);
		AccumulateScreenPos(Info);
	}
	else
	{
		DebugLine(Info, DebugTitle, 0.f, 0.f);
	}
}

void FGameplayDebuggerCategory_OUUAbilities::DrawDebugHeader(
	FAbilitySystemComponentDebugInfo& Info,
	UOUUAbilitySystemComponent* AbilitySystem)
{
	FString DebugTitle("");
	AActor* LocalAvatarActor = AbilitySystem->GetAvatarActor_Direct();
	AActor* LocalOwnerActor = AbilitySystem->GetOwnerActor();

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

	DrawTitle(Info, DebugTitle);
	DrawTitle(Info, "");
}

void FGameplayDebuggerCategory_OUUAbilities::AccumulateScreenPos(FAbilitySystemComponentDebugInfo& Info)
{
	const float NewY = Info.YPos + Info.YL;
	if (NewY > Info.MaxY)
	{
		NewColumn(Info);
	}
	else
	{
		Info.YPos = NewY;
	}
}

void FGameplayDebuggerCategory_OUUAbilities::NewColumn(FAbilitySystemComponentDebugInfo& Info) const
{
	Info.YPos = Info.NewColumnYPadding;
	const float MaxX = Info.Canvas ? Info.Canvas->ClipX : 0.f;
	const float ColumnWidth = Info.Canvas ? Info.Canvas->ClipX * (1.f / NumColumns) : 0.f;
	Info.XPos += ColumnWidth;
	if (Info.XPos > MaxX)
	{
		Info.Canvas->SetDrawColor(FColor::Yellow);
		FFontRenderInfo RenderInfo = FFontRenderInfo();
		RenderInfo.bEnableShadow = true;
		const UFont* LargeFont = GEngine->GetLargeFont();
		Info.Canvas->DrawText(
			LargeFont,
			"COLUMN OVERSPILL",
			Info.XPos + 4.f,
			Info.YPos - LargeFont->GetMaxCharHeight(),
			1.f,
			1.f,
			RenderInfo);
	}
}

void FGameplayDebuggerCategory_OUUAbilities::NewColumnForCategory_Optional(FAbilitySystemComponentDebugInfo& Info) const
{
	if (Info.YPos > Info.NewColumnYPadding + (Info.MaxY - Info.NewColumnYPadding) / 0.6f)
	{
		NewColumn(Info);
	}
}

void FGameplayDebuggerCategory_OUUAbilities::DebugLine(
	FAbilitySystemComponentDebugInfo& Info,
	FString Str,
	float XOffset,
	float YOffset,
	int32 MinTextRowsToAdvance)
{
	if (Info.Canvas)
	{
		FFontRenderInfo RenderInfo = FFontRenderInfo();
		RenderInfo.bEnableShadow = true;
		if (const UFont* Font = GEngine->GetTinyFont())
		{
			Info.YL = Info.Canvas->DrawText(Font, Str, Info.XPos + XOffset, Info.YPos, 1.f, 1.f, RenderInfo);
			Info.YL = FMath::Max(Info.YL, MinTextRowsToAdvance * Font->GetMaxCharHeight());
			AccumulateScreenPos(Info);
		}
	}

	if (Info.bPrintToLog)
	{
		FString LogStr;
		for (int32 i = 0; i < (int32)XOffset; ++i)
		{
			LogStr += TEXT(" ");
		}
		LogStr += Str;
		ABILITY_LOG(Warning, TEXT("%s"), *LogStr);
	}

	if (Info.Accumulate)
	{
		FString LogStr;
		for (int32 i = 0; i < (int32)XOffset; ++i)
		{
			LogStr += TEXT(" ");
		}
		LogStr += Str;
		Info.Strings.Add(Str);
	}
}

void FGameplayDebuggerCategory_OUUAbilities::AddTagList(
	FAbilitySystemComponentDebugInfo& Info,
	UOUUAbilitySystemComponent* AbilitySystem,
	FGameplayTagContainer Tags,
	FString TagsListTitle)
{
	int32 TagCount = 1;
	int32 NumTags = Tags.Num();
	FString CombinedTagsString = "";
	for (FGameplayTag Tag : Tags)
	{
		CombinedTagsString.Append(FString::Printf(TEXT("\n%s (%d)"), *Tag.ToString(), AbilitySystem->GetTagCount(Tag)));

		if (TagCount++ < NumTags)
		{
			CombinedTagsString += TEXT(", ");
		}
	}

	DebugLine(Info, FString::Printf(TEXT("%s: %s"), *TagsListTitle, *CombinedTagsString), 4.f, 0.f, 2);
	DebugLine(Info, "", 0.f, 0.f, 2);
}

#endif
