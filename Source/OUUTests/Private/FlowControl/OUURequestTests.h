// Copyright (c) 2021 Jonas Reich

#include "CoreMinimal.h"

#include "FlowControl/OUURequest.h"
#include "OUURequestTests.generated.h"

UCLASS(meta = (Hidden, HideDropDown))
class UOUURequestTests_Responder : public UObject
{
	GENERATED_BODY()
public:
	bool bCompleteRequestSuccesfully = false;

	UFUNCTION()
	void HandleRequestRaised(UOUURequest* Request)
	{
		Request->Complete(bCompleteRequestSuccesfully);
	}
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
	void HandleStateChanged(UOUURequest* Request, EOUURequestState State)
	{
		StateHistory.Add(State);
	}
};

UCLASS(meta = (Hidden, HideDropDown))
class UOUURequestQueue_TestRequest : public UOUURequest
{
	GENERATED_BODY()
public:

};
