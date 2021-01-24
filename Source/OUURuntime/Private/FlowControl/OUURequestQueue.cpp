// Copyright (c) 2021 Jonas Reich

#include "FlowControl/OUURequestQueue.h"

UOUURequestQueue::UOUURequestQueue()
{
	RequestClass = UOUURequest::StaticClass();
}

UOUURequest* UOUURequestQueue::CreateNewRequest()
{
	UClass* RequestClassToUse = ensure(RequestClass) ? RequestClass : UOUURequest::StaticClass();
	UOUURequest* Request = NewObject<UOUURequest>(GetTransientPackage(), RequestClassToUse);
	RequestQueue.Add(Request);
	Request->OnCompleted.AddDynamic(this, &UOUURequestQueue::HandleRequestCompleted);
	Request->OnRaised.AddDynamic(this, &UOUURequestQueue::HandleRequestRaised);
	return Request;
}

UOUURequest* UOUURequestQueue::RaiseNewRequest()
{
	UOUURequest* Request = CreateNewRequest();
	Request->Raise();
	return Request;
}

UOUURequest* UOUURequestQueue::RaiseNewRequestAndWait(FOnRequestStatusChangedDelegate CompletedCallback)
{
	UOUURequest* Request = CreateNewRequest();
	Request->RaiseAndWait(CompletedCallback);
	return Request;
}

TArray<UOUURequest*> UOUURequestQueue::GetRequestsInQueue()
{
	return RequestQueue;
}

UOUURequest* UOUURequestQueue::GetOldestRequest() const
{
	return RequestQueue.Num() > 0 ? RequestQueue[0] : nullptr;
}

UOUURequest* UOUURequestQueue::GetOldestRequestWithState(EOUURequestState State) const
{
	if (UOUURequest*const* ResultPtr = RequestQueue.FindByPredicate([State](const UOUURequest* R) -> bool
	{
		return IsValid(R) && R->GetState() == State;
	}))
	{
		return *ResultPtr;
	}
	return nullptr;
}

void UOUURequestQueue::HandleRequestRaised(UOUURequest* Request)
{
	// Forward newly raised requests
	OnRequestRaised.Broadcast(Request);
}

void UOUURequestQueue::HandleRequestCompleted(UOUURequest* Request, EOUURequestState State)
{
	Request->OnCompleted.RemoveDynamic(this, &UOUURequestQueue::HandleRequestCompleted);
	Request->OnRaised.RemoveDynamic(this, &UOUURequestQueue::HandleRequestRaised);
	RequestQueue.Remove(Request);
	OnCompleted.Broadcast(Request, State);
}
