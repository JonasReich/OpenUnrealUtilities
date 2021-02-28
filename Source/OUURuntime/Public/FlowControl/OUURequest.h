// Copyright (c) 2021 Jonas Reich

#pragma once

#include "CoreMinimal.h"
#include "OUURequest.generated.h"

class UOUURequest;

/** The current (exclusive) state of a request object */
UENUM(BlueprintType)
enum class EOUURequestState : uint8
{
	// Default state after construction
	Idle,
	// The request has been raised and is waiting for fulfillment
	Pending,
	// The request was dropped by the caller
	Canceled,
	// The request was successfully fulfilled
	Successful,
	// The request could not be successfully fulfilled
	Failed
};

FString OUURUNTIME_API LexToString(EOUURequestState State);

DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnRequestStatusChangedDelegate, UOUURequest*, Request, EOUURequestState, State);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRequestStatusChanged, UOUURequest*, Request, EOUURequestState, State);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnRequestRaisedDelegate, UOUURequest*, Request);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRequestRaised, UOUURequest*, Request);

/**
 * Request object that represents a request from one component to another.
 * One possible application for requests are blueprint callbacks where a C++ system makes a request and binds to the OnCompleted delegate
 * and a blueprint fulfills the request, which calls the previously bound callback. 
 * 
 * Requests can succeed, fail or be canceled and may be reset and reused after completion.
 * A request can be triggered multiple times from multiple sources, but will only propagate the first of these calls.
 * This is useful in many situations, e.g. save requests. If this is not sufficient and you need a request that will be acted on for each
 * individual call, consider using a UOUURequestQueue instead.
 * 
 * Request payload data should be added as class members of child classes and can be set up with bidirectional read/write access
 * (from caller to responder and vice versa).
 */
UCLASS(BlueprintType, Blueprintable)
class OUURUNTIME_API UOUURequest : public UObject
{
	GENERATED_BODY()
public:
	/** Called on every status transition of the request */
	UPROPERTY(BlueprintAssignable)
	FOnRequestStatusChanged OnStatusChanged;

	/**
	 * Called whenever the request is raised. Only called on the initial request.
	 * Will not be called again for consecutive calls of Raise().
	 */
	UPROPERTY(BlueprintAssignable)
	FOnRequestRaised OnRaised;

	/** Called whenever the request is fulfilled or canceled */
	UPROPERTY(BlueprintAssignable)
	FOnRequestStatusChanged OnCompleted;

	/**
	 * Should the status be reset to Idle automatically after completion?
	 * If this is unchecked, the request has to be reset manually via Reset();
	 */
	UPROPERTY(EditDefaultsOnly)
	bool bResetAfterCompletion = true;

	/** Raises the request without binding a callback delegate */
	UFUNCTION(BlueprintCallable)
	void Raise();

	/** Raise the request and bind a callback delegate that will be called when the request is completed */
	UFUNCTION(BlueprintCallable)
	void RaiseAndWait(FOnRequestStatusChangedDelegate CompletedCallback);
	
	/**
	 * Drop the request from the caller side. (i.e. "never mind, I don't need this anymore")
	 * Doesn't do anything if the request is not in the pending state.
	 */
	UFUNCTION(BlueprintCallable)
	void Cancel();

	/**
	 * Mark the request as completed. Can be either success or failure.
	 * Doesn't do anything if the request is not in the pending state.
	 */
	UFUNCTION(BlueprintCallable)
	void Complete(bool bSuccessful);

	/** Reset the request status after completion. Doesn't do anything while the request is still pending. */
	UFUNCTION(BlueprintCallable)
	void Reset();
	
	/**
	 * Get the current state of the request. When multiple objects bind to the OnRaised event, it's useful to check if the request has
	 * already been fulfilled by a third party.
	 */
	UFUNCTION(BlueprintPure)
	EOUURequestState GetState() const;

private:
	EOUURequestState State = EOUURequestState::Idle;

	void ChangeState(EOUURequestState NewState);
};
