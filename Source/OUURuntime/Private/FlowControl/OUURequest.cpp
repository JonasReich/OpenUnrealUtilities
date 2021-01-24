// Copyright (c) 2021 Jonas Reich

#include "FlowControl/OUURequest.h"

FString LexToString(EOUURequestState E)
{
	switch (E)
	{
	case EOUURequestState::Idle: return TEXT("Idle");
	case EOUURequestState::Pending: return TEXT("Pending");
	case EOUURequestState::Canceled: return TEXT("Canceled");
	case EOUURequestState::Successful: return TEXT("Successful");
	case EOUURequestState::Failed: return TEXT("Failed");
	default: return FString();
	}
}

void UOUURequest::Raise()
{
	if (State != EOUURequestState::Idle)
		return;

	ChangeState(EOUURequestState::Pending);
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
	if (State == NewState)
		return;
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
