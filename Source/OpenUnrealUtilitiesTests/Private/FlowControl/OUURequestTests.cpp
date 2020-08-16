// Copyright (c) 2020 Jonas Reich

#include "OUURequestTests.h"
#include "OUUTests.h"

#if WITH_AUTOMATION_WORKER

#include "FlowControl/OUURequest.h"

#define OUU_TEST_CATEGORY OpenUnrealUtilities
#define OUU_TEST_TYPE OUURequest

// Shared environment for all tests
struct FOUURequestTestEnvironment
{
	// The request itself
	UOUURequest* Request;
	// One object acting as the owner, i.e. the one raising the request
	UOUURequestTests_Owner* Owner;
	// One object acting as the responder, i.e. the one who reacts to the request
	UOUURequestTests_Responder* Responder;

	FOUURequestTestEnvironment()
	{
		Request = NewObject<UOUURequest>();
		Owner = NewObject<UOUURequestTests_Owner>();
		Responder = NewObject<UOUURequestTests_Responder>();
	}
};

//////////////////////////////////////////////////////////////////////////

/**
 * 0: success / fail
 * 1: reset after completion
 * 2: raise and wait / raise
 * 3: manually reset?
 */
OUU_IMPLEMENT_COMPLEX_AUTOMATION_TEST_BEGIN(Raise, DEFAULT_OUU_TEST_FLAGS)
OUU_COMPLEX_AUTOMATION_TESTCASE("1|1|1|0")
OUU_COMPLEX_AUTOMATION_TESTCASE("0|1|1|0")
OUU_COMPLEX_AUTOMATION_TESTCASE("1|0|1|0")
OUU_COMPLEX_AUTOMATION_TESTCASE("0|0|1|0")
OUU_COMPLEX_AUTOMATION_TESTCASE("1|1|0|0")
OUU_COMPLEX_AUTOMATION_TESTCASE("0|1|0|0")
OUU_COMPLEX_AUTOMATION_TESTCASE("1|0|0|0")
OUU_COMPLEX_AUTOMATION_TESTCASE("0|0|0|0")
OUU_COMPLEX_AUTOMATION_TESTCASE("1|1|1|1")
OUU_COMPLEX_AUTOMATION_TESTCASE("0|1|1|1")
OUU_COMPLEX_AUTOMATION_TESTCASE("1|0|1|1")
OUU_COMPLEX_AUTOMATION_TESTCASE("0|0|1|1")
OUU_COMPLEX_AUTOMATION_TESTCASE("1|1|0|1")
OUU_COMPLEX_AUTOMATION_TESTCASE("0|1|0|1")
OUU_COMPLEX_AUTOMATION_TESTCASE("1|0|0|1")
OUU_COMPLEX_AUTOMATION_TESTCASE("0|0|0|1")
OUU_IMPLEMENT_COMPLEX_AUTOMATION_TEST_END(Raise)
{
	// Arrange
	FTestParameterParser Parser{ Parameters };
	FOUURequestTestEnvironment Env;
	const bool bExpectSuccess = Parser.GetValue<bool>(0);
	Env.Responder->bCompleteRequestSuccesfully = bExpectSuccess;
	const bool bResetAfterCompletion = Parser.GetValue<bool>(1);
	Env.Request->bResetAfterCompletion = bResetAfterCompletion;
	const bool bRaiseAndWait = Parser.GetValue<bool>(2);
	const bool bResetManually = Parser.GetValue<bool>(3);
	
	Env.Request->OnRaised.AddDynamic(Env.Responder, &UOUURequestTests_Responder::HandleRequestRaised);

	// Act
	if (bRaiseAndWait)
	{
		FOnRequestStatusChangedDelegate Callback;
		Callback.BindDynamic(Env.Owner, &UOUURequestTests_Owner::HandleRequestCompleted);
		Env.Request->RaiseAndWait(Callback);
	}
	else
	{
		Env.Request->OnCompleted.AddDynamic(Env.Owner, &UOUURequestTests_Owner::HandleRequestCompleted);
		Env.Request->Raise();
	}

	if (bResetManually)
	{
		Env.Request->Reset();
	}

	// Assert
	TestTrue("Callback was received", Env.Owner->bRequestCompleted);
	EOUURequestState ExpectedCallbackState = bExpectSuccess ? EOUURequestState::Successful : EOUURequestState::Failed;
	TestEqual("Success State (callback)", Env.Owner->StateAfterCompletion, ExpectedCallbackState);
	EOUURequestState ExpectedRequestState = (bResetAfterCompletion || bResetManually) ? EOUURequestState::Idle : ExpectedCallbackState;
	TestEqual("Success State (request)", Env.Request->GetState(), ExpectedRequestState);

	return true;
}

//////////////////////////////////////////////////////////////////////////

#undef OUU_TEST_CATEGORY
#undef OUU_TEST_TYPE

#endif
