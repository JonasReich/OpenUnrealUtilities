// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "FlowControl/OUURequest.h"

#include "OUURequestTests.generated.h"

UCLASS(meta = (Hidden, HideDropDown))
class UOUURequestTests_Responder : public UObject
{
	GENERATED_BODY()
public:
	bool bCompleteRequestSuccessfully = false;

	UFUNCTION()
	// ReSharper disable once CppMemberFunctionMayBeConst
	void HandleRequestRaised(UOUURequest* Request) { Request->Complete(bCompleteRequestSuccessfully); }
};

UCLASS(meta = (Hidden, HideDropDown))
class UOUURequestTests_Owner : public UObject
{
	GENERATED_BODY()
public:
	bool bRequestCompleted = false;
	EOUURequestState StateAfterCompletion = EOUURequestState::Idle;

	UFUNCTION()
	void HandleRequestCompleted(UOUURequest* Request, EOUURequestState State)
	{
		bRequestCompleted = true;
		StateAfterCompletion = State;
	}

	TArray<EOUURequestState> StateHistory;
	UFUNCTION()
	void HandleStateChanged(UOUURequest* Request, EOUURequestState State) { StateHistory.Add(State); }
};

UCLASS(meta = (Hidden, HideDropDown))
class UOUURequestQueue_TestRequest : public UOUURequest
{
	GENERATED_BODY()
};
