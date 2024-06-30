// Copyright (c) 2024 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "AlphaBlend.h"
#include "GameFramework/Actor.h"

#include "OUUSyncedGameTimeActor.generated.h"

class AGameStateBase;

USTRUCT()
struct FOUUSyncedGameTimeBlend
{
	GENERATED_BODY()
public:
	UPROPERTY()
	double BlendServerStartTime = 0.0;

	UPROPERTY()
	double BlendDuration = 0.0;

	UPROPERTY()
	EAlphaBlendOption BlendOption = EAlphaBlendOption::Linear;

	UPROPERTY()
	double TimeOffsetAfterBlend = 0.0;

	// Filled in by the time actor
	UPROPERTY()
	double TimeOffsetBeforeBlend = 0.0;

public:
	double GetBlendedTimeOffset(double ServerTime) const;
};

/**
 * Actor to keep track of a synced game time based off server game time
 * (e.g. time of day systems). The actor tracks the time in absolute value and "cycles" which depending on your game
 * might be days, years, etc. whatever you need for gameplay purposes.
 *
 * Note: The actor is not an AInfo actor, because for TQ2 we wanted to use the same actor for weather and time of day
 * visualization (light components, etc).
 */
UCLASS()
class OUURUNTIME_API AOUUSyncedGameTimeActor : public AActor
{
	GENERATED_BODY()
public:
	AOUUSyncedGameTimeActor();

	UFUNCTION(BlueprintPure)
	double GetCurrentTimeSeconds() const;

	// Set a new game time without blending
	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable)
	void SetCurrentTimeSeconds(double NewGameTime);

	// Set the game time with a client synchronized blend.
	// - Start blending at the current game server time
	// - Blend for the given duration with BlendOption as blending operation
	// - After the blend is completed it will be the new game time
	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable)
	void SetCurrentTimeSecondsWithBlend(double GameTimeAfterBlend, double BlendDuration, EAlphaBlendOption BlendOption);

	UFUNCTION(BlueprintPure)
	double GetScaledCycleDurationSeconds() const;

	UFUNCTION(BlueprintPure)
	double GetNormalizedTime() const;

	UFUNCTION(BlueprintPure)
	double GetOverrideTimeScale() const;

	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable)
	void SetOverrideTime(bool bEnableOverride, double NewOverrideTimeInSeconds);

	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable)
	void SetOverrideTimeScale(double NewTimeScale);

	// - AActor
public:
	void BeginPlay() override;

#if WITH_EDITOR
	void PostLoad() override;
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	// --

	void RefreshInitialTime();

	// IMPLEMENT ME: Get the unscaled duration of a game time cycle in seconds.
	// This is relevant to determine normalized time.
	// NOTE: Derived classes need to make sure this returns the same value on client and server!
	virtual double GetUnscaledCycleDurationSeconds() const;

	// IMPLEMENT ME: Get the unscaled duration of a game time cycle in seconds
	// This only needs to be called on the server, so no need for client synchronization.
	virtual double GetInitialTimeSeconds() const;

private:
	// These members are private to ensure push model replication isn't accidentally broken.

	// Override the timescale (multiplier) how fast cycles elapse.
	UPROPERTY(VisibleAnywhere, Replicated, Transient)
	double OverrideTimeScale = 1.0;

	// Override time value for debugging and editor pre-vis.
	// This one value combines the actual time override (if positive) and deactivation state (if negative).
	UPROPERTY(VisibleAnywhere, Replicated, Transient)
	double OverrideTime = -1.0;

	UPROPERTY(Replicated, Transient, ReplicatedUsing = "OnRep_TimeBlend")
	FOUUSyncedGameTimeBlend TimeBlend;

	UPROPERTY(Transient)
	AGameStateBase* CachedGameState = nullptr;

	UFUNCTION()
	void OnRep_TimeBlend(FOUUSyncedGameTimeBlend OldBlend);
};
