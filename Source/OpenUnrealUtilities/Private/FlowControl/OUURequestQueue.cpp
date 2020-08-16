// Copyright (c) 2020 Jonas Reich

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
