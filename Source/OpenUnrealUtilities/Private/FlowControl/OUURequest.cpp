// Copyright (c) 2020 Jonas Reich

#include "FlowControl/OUURequest.h"

void UOUURequest::Raise()
{
	if (State != EOUURequestState::Idle)
		return;

	State = EOUURequestState::Pending;
	OnRaised.Broadcast(this);
}

void UOUURequest::RaiseAndWait(FOnRequestStatusChangedDelegate CompletedCallback)
{
	OnCompleted.AddUnique(CompletedCallback);
	Raise();
}

void UOUURequest::Cancel()
{
	if (State != EOUURequestState::Pending)
		return;

	ChangeState(EOUURequestState::Canceled);
}

void UOUURequest::Complete(bool bSuccessful)
{
	if (State != EOUURequestState::Pending)
		return;

	ChangeState(bSuccessful ? EOUURequestState::Successful : EOUURequestState::Failed);
}

void UOUURequest::Reset()
{
	if (State == EOUURequestState::Pending || State == EOUURequestState::Idle)
		return;

	ChangeState(EOUURequestState::Idle);
}

EOUURequestState UOUURequest::GetState() const
{
	return State;
}

void UOUURequest::ChangeState(EOUURequestState NewState)
{
	State = NewState;
	OnStatusChanged.Broadcast(this, NewState);
	if (NewState == EOUURequestState::Successful ||
		NewState == EOUURequestState::Failed ||
		NewState == EOUURequestState::Canceled)
	{
		OnCompleted.Broadcast(this, NewState);
		if (bResetAfterCompletion)
		{
			Reset();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
 
UOUURequest* UOUURequestQueue::CreateNewRequest()
{
	return nullptr;
}

UOUURequest* UOUURequestQueue::RaiseNewRequest()
{
	return nullptr;
}

UOUURequest* UOUURequestQueue::RaiseNewRequestAndWait(FOnRequestStatusChangedDelegate CompletedCallback)
{
	return nullptr;
}

TArray<UOUURequest*> UOUURequestQueue::GetPendingRequests()
{
	return RequestQueue;
}

void UOUURequestQueue::HandleRequestRaised(UOUURequest* Request) const
{

}

void UOUURequestQueue::HandleRequestCompleted(UOUURequest* Request, EOUURequestState State) const
{

}
