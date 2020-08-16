// Copyright (c) 2020 Jonas Reich

#include "CoreMinimal.h"
#include "FlowControl/OUURequest.h"
#include "OUURequestTests.generated.h"

UCLASS()
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

UCLASS()
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
