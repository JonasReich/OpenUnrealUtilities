// Copyright (c) 2024 Jonas Reich & Contributors

#include "Misc/OUUSyncedGameTimeActor.h"

#include "AlphaBlend.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Net/UnrealNetwork.h"

//---------------------------------------------------------------------------------------------------------------------
// FOUUSyncedGameTimeBlend
//---------------------------------------------------------------------------------------------------------------------
double FOUUSyncedGameTimeBlend::GetBlendedTimeOffset(double ServerTime) const
{
	const double BlendEndTime = BlendServerStartTime + BlendDuration;
	const double LinearBlendAlpha =
		FMath::Clamp(FMath::GetRangePct(BlendServerStartTime, BlendEndTime, ServerTime), 0.0, 1.0);
	// Custom curves not supported atm
	constexpr UCurveFloat* CustomCurve = nullptr;
	FAlphaBlend::AlphaToBlendOption(LinearBlendAlpha, BlendOption, CustomCurve);

	return FMath::Lerp(TimeOffsetBeforeBlend, TimeOffsetAfterBlend, LinearBlendAlpha);
}

//---------------------------------------------------------------------------------------------------------------------
// AOUUSyncedGameTimeActor
//---------------------------------------------------------------------------------------------------------------------
AOUUSyncedGameTimeActor::AOUUSyncedGameTimeActor()
{
	bReplicates = true;
	NetUpdateFrequency = 1.0f;

	PrimaryActorTick.bCanEverTick = true;

#if WITH_EDITORONLY_DATA
	// A game time actor should never be streamed
	bIsSpatiallyLoaded = false;
#endif
}

double AOUUSyncedGameTimeActor::GetCurrentTimeSeconds() const
{
	if (OverrideTime > 0.0)
	{
		return OverrideTime;
	}
	const double CurrentServerTime = CachedGameState ? CachedGameState->GetServerWorldTimeSeconds() : 0.0;
	return CurrentServerTime + TimeBlend.GetBlendedTimeOffset(CurrentServerTime);
}

void AOUUSyncedGameTimeActor::SetCurrentTimeSeconds(double NewGameTime)
{
	SetCurrentTimeSecondsWithBlend(NewGameTime, 0.0, EAlphaBlendOption::Linear);
}

void AOUUSyncedGameTimeActor::SetCurrentTimeSecondsWithBlend(
	double GameTimeAfterBlend,
	double BlendDuration,
	EAlphaBlendOption BlendOption)
{
	const double CurrentServerTime = CachedGameState ? CachedGameState->GetServerWorldTimeSeconds() : 0.0;
	const double ServerTimeAfterBlend = CurrentServerTime + BlendDuration;
	const double NewTimeOffset = GameTimeAfterBlend - ServerTimeAfterBlend;

	// The current time offset needs to be saved before we assign any new values to the blend!
	const auto CurrentTimeOffset = TimeBlend.GetBlendedTimeOffset(CurrentServerTime);

	TimeBlend.BlendServerStartTime = CurrentServerTime;
	// Make sure the perceived time doesn't jump when entering a blend while a previous blend is in-progress
	TimeBlend.TimeOffsetBeforeBlend = CurrentTimeOffset;
	TimeBlend.TimeOffsetAfterBlend = NewTimeOffset;
	TimeBlend.BlendDuration = BlendDuration;
	TimeBlend.BlendOption = BlendOption;
	MARK_PROPERTY_DIRTY_FROM_NAME(AOUUSyncedGameTimeActor, TimeBlend, this);
}

double AOUUSyncedGameTimeActor::GetScaledCycleDurationSeconds() const
{
	return FMath::IsNearlyZero(OverrideTimeScale) ? GetUnscaledCycleDurationSeconds()
												  : (GetUnscaledCycleDurationSeconds() / OverrideTimeScale);
}

double AOUUSyncedGameTimeActor::GetUnscaledCycleDurationSeconds() const
{
	// This is meant to be extended by derived classes.
	// For a demo implementation we'll just use 60 seconds.
	return 60.0f;
}

double AOUUSyncedGameTimeActor::GetInitialTimeSeconds() const
{
	return 0.f;
}

double AOUUSyncedGameTimeActor::GetNormalizedTime() const
{
	return FMath::Frac(GetCurrentTimeSeconds() / GetScaledCycleDurationSeconds());
}

double AOUUSyncedGameTimeActor::GetOverrideTimeScale() const
{
	return OverrideTimeScale;
}

void AOUUSyncedGameTimeActor::SetOverrideTime(bool bEnableOverride, double NewOverrideTimeInSeconds)
{
	const double NewOverrideTime = bEnableOverride ? FMath::Abs(NewOverrideTimeInSeconds) : -1.0;
	COMPARE_ASSIGN_AND_MARK_PROPERTY_DIRTY(AOUUSyncedGameTimeActor, OverrideTime, NewOverrideTime, this);
}

void AOUUSyncedGameTimeActor::SetOverrideTimeScale(double NewTimeScale)
{
	const bool bWasPaused = FMath::IsNearlyZero(OverrideTimeScale);
	const bool bShouldPause = FMath::IsNearlyZero(NewTimeScale);

	const double PrevScale = bWasPaused ? 1.0 : OverrideTimeScale;
	const double UnScaledCurrentTime = GetCurrentTimeSeconds() * PrevScale;
	OverrideTimeScale = NewTimeScale;
	const double NewScale = FMath::IsNearlyZero(OverrideTimeScale) ? 1.0 : OverrideTimeScale;
	const double NewTime = UnScaledCurrentTime / NewScale;
	SetCurrentTimeSeconds(NewTime);
	if (bShouldPause)
	{
		SetOverrideTime(true, NewTime);
	}
	else
	{
		SetOverrideTime(false, -1.0 /*discarded*/);
	}
}

void AOUUSyncedGameTimeActor::BeginPlay()
{
	Super::BeginPlay();

	const auto* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	CachedGameState = World->GetGameState();
	COMPARE_ASSIGN_AND_MARK_PROPERTY_DIRTY(AOUUSyncedGameTimeActor, OverrideTime, -1.0, this);
	COMPARE_ASSIGN_AND_MARK_PROPERTY_DIRTY(AOUUSyncedGameTimeActor, OverrideTimeScale, 1.0, this);

	if (HasAuthority())
	{
		RefreshInitialTime();
	}
}

#if WITH_EDITOR
void AOUUSyncedGameTimeActor::PostLoad()
{
	Super::PostLoad();
	RefreshInitialTime();
}

void AOUUSyncedGameTimeActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	RefreshInitialTime();
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void AOUUSyncedGameTimeActor::RefreshInitialTime()
{
	SetCurrentTimeSeconds(GetInitialTimeSeconds());
}

void AOUUSyncedGameTimeActor::OnRep_TimeBlend(FOUUSyncedGameTimeBlend OldBlend)
{
	if (CachedGameState)
	{
		// Use the previous local time offset as foundation for blend to avoid noticeable jumps after replication,
		// which will inevitably be delayed by a few frames.
		const auto ServerTime = CachedGameState->GetServerWorldTimeSeconds();
		TimeBlend.TimeOffsetBeforeBlend = OldBlend.GetBlendedTimeOffset(ServerTime);
	}
}

void AOUUSyncedGameTimeActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS(AOUUSyncedGameTimeActor, TimeBlend, Params)
	DOREPLIFETIME_WITH_PARAMS(AOUUSyncedGameTimeActor, OverrideTime, Params)
	DOREPLIFETIME_WITH_PARAMS(AOUUSyncedGameTimeActor, OverrideTimeScale, Params)
}
