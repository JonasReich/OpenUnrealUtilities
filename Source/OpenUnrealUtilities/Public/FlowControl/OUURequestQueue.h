// Copyright (c) 2020 Jonas Reich

#pragma once

#include "CoreMinimal.h"
#include "FlowControl/OUURequest.h"
#include "OUURequestQueue.generated.h"

/**
 * Container for raising multiple requests of the same type consecutively that should be fulfilled independently of one another.
 * Request objects are dropped after completion and may be garbage collected.
 */
UCLASS(BlueprintType, Blueprintable)
class OPENUNREALUTILITIES_API UOUURequestQueue : public UObject
{
	GENERATED_BODY()
public:
	UOUURequestQueue();

	/** Called every time a request is raised */
	UPROPERTY(BlueprintAssignable)
	FOnRequestRaised OnRequestRaised;

	/** Called whenever a request is fulfilled or canceled and removed from the queue */
	UPROPERTY(BlueprintAssignable)
	FOnRequestStatusChanged OnCompleted;

	/** Class to use as template for new requests */
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UOUURequest> RequestClass;

	/**
	 * Raise a new request. Adds a new element to the queue.
	 * The request must be manually raised by the caller!
	 * This allows setting payload data on the request before calling.
	 * @returns newly created request
	 */
	UFUNCTION(BlueprintCallable)
	UOUURequest* CreateNewRequest();

	/**
	 * Raise a new request. Adds a new element to the queue.
	 * Also immediately raises the request, which prevents payload data from being set before the request is raised.
	 * Only use for request types that do not need any payload data!
	 * @returns newly created request
	 */
	UFUNCTION(BlueprintCallable)
	UOUURequest* RaiseNewRequest();

	/**
	 * Raise a new request. Adds a new element to the queue.
	 * Also immediately raises the request, which prevents payload data from being set before the request is raised.
	 * Only use for request types that do not need any payload data!
	 * @param CompletedCallback: Callback delegate that will be called when the request is completed
	 * @returns newly created request
	 */
	UFUNCTION(BlueprintCallable)
	UOUURequest* RaiseNewRequestAndWait(FOnRequestStatusChangedDelegate CompletedCallback);

	/**
	 * Get a list of all requests in the queue.
	 * Items are sorted by the time the individual requests were created.
	 * Requests may be in idle state and not raised!
	 */
	UFUNCTION(BlueprintCallable)
	TArray<UOUURequest*> GetRequestsInQueue();

private:
	UPROPERTY(Transient)
	TArray<UOUURequest*> RequestQueue;

	// React to one of the requests that were created in the queue being raised.
	UFUNCTION()
	void HandleRequestRaised(UOUURequest* Request);

	// React to one of the requests that were created in the queue being completed.
	UFUNCTION()
	void HandleRequestCompleted(UOUURequest* Request, EOUURequestState State);
};
