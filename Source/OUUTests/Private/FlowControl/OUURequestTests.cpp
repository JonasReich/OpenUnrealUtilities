// Copyright (c) 2021 Jonas Reich

#include "OUURequestTests.h"
#include "OUUTestUtilities.h"

#if WITH_AUTOMATION_WORKER

#include "FlowControl/OUURequest.h"
#include "FlowControl/OUURequestQueue.h"

#define OUU_TEST_CATEGORY OpenUnrealUtilities.FlowControl

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
// Single Request Tests
//////////////////////////////////////////////////////////////////////////

#define OUU_TEST_TYPE OUURequest

/**
 * 0: success / fail
 * 1: reset after completion
 * 2: raise and wait / raise
 * 3: manually reset?
 * 4: raise multiple times?
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
	FAutomationTestParameterParser Parser{ Parameters };
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

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(RaiseTwice, DEFAULT_OUU_TEST_FLAGS)
{
	// Arrange
	FOUURequestTestEnvironment Env;
	Env.Responder->bCompleteRequestSuccesfully = true;
	Env.Request->bResetAfterCompletion = true;
	Env.Request->OnCompleted.AddDynamic(Env.Owner, &UOUURequestTests_Owner::HandleRequestCompleted);

	// Act
	Env.Request->Raise();
	Env.Request->Raise();
	Env.Request->Complete(true);

	// Assert
	TestTrue("Callback was received", Env.Owner->bRequestCompleted);
	TestEqual("Success State (callback)", Env.Owner->StateAfterCompletion, EOUURequestState::Successful);
	TestEqual("Success State (request)", Env.Request->GetState(), EOUURequestState::Idle);
	
	return true;
}

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(Cancel, DEFAULT_OUU_TEST_FLAGS)
{
	// Arrange
	FOUURequestTestEnvironment Env;
	Env.Responder->bCompleteRequestSuccesfully = true;
	Env.Request->bResetAfterCompletion = true;
	Env.Request->OnCompleted.AddDynamic(Env.Owner, &UOUURequestTests_Owner::HandleRequestCompleted);

	// Act
	Env.Request->Raise();
	Env.Request->Cancel();
	Env.Request->Complete(true);

	// Assert
	TestTrue("Callback was received", Env.Owner->bRequestCompleted);
	TestEqual("Success State (callback)", Env.Owner->StateAfterCompletion, EOUURequestState::Canceled);
	TestEqual("Success State (request)", Env.Request->GetState(), EOUURequestState::Idle);

	return true;
}

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(OnStatusChanged, DEFAULT_OUU_TEST_FLAGS)
{
	// Arrange
	FOUURequestTestEnvironment Env;
	Env.Responder->bCompleteRequestSuccesfully = true;
	Env.Request->bResetAfterCompletion = false;
	Env.Request->OnStatusChanged.AddDynamic(Env.Owner, &UOUURequestTests_Owner::HandleStateChanged);

	// Act
	Env.Request->Raise();
	Env.Request->Cancel();
	Env.Request->Reset();
	Env.Request->Raise();
	Env.Request->Complete(true);
	Env.Request->Reset();
	Env.Request->Raise();
	Env.Request->Complete(false);

	// Assert
	TArray<EOUURequestState> ExpectedHistory = {
		EOUURequestState::Pending,
		EOUURequestState::Canceled,
		EOUURequestState::Idle,
		EOUURequestState::Pending,
		EOUURequestState::Successful,
		EOUURequestState::Idle,
		EOUURequestState::Pending,
		EOUURequestState::Failed
	};
	TestArraysEqual(*this, "State history", Env.Owner->StateHistory, ExpectedHistory);

	return true;
}

//////////////////////////////////////////////////////////////////////////
// Request Queue tests
//////////////////////////////////////////////////////////////////////////

#undef OUU_TEST_TYPE
#define OUU_TEST_TYPE OUURequestQueue

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(CreateNewRequest, DEFAULT_OUU_TEST_FLAGS)
{
	// Arrange
	UOUURequestQueue* RequestQueue = NewObject<UOUURequestQueue>();
	RequestQueue->RequestClass = UOUURequestQueue_TestRequest::StaticClass();

	// Act
	UOUURequest* Request0 = RequestQueue->CreateNewRequest();
	UOUURequest* Request1 = RequestQueue->CreateNewRequest();
	TArray<UOUURequest*> OpenRequests = RequestQueue->GetRequestsInQueue();
	UOUURequest* OldestRequest = RequestQueue->GetOldestRequest();
	UOUURequest* OldestPendingRequest = RequestQueue->GetOldestRequestWithState(EOUURequestState::Pending);
	UOUURequest* OldestIdleRequest = RequestQueue->GetOldestRequestWithState(EOUURequestState::Idle);

	// Assert
	bool bRequest0CorrectClass = IsValid(Cast<UOUURequestQueue_TestRequest>(Request0));
	bool bRequest1CorrectClass = IsValid(Cast<UOUURequestQueue_TestRequest>(Request1));
	bool bDifferentObjects = Request0 != Request1;
	TArray<UOUURequest*> ExpectedRequestList = { Request0, Request1 };

	TestTrue("Request 0 has correct class", bRequest0CorrectClass);
	TestTrue("Request 1 has correct class", bRequest1CorrectClass);
	TestEqual("Request 0 state", Request0->GetState(), EOUURequestState::Idle);
	TestEqual("Request 1 state", Request1->GetState(), EOUURequestState::Idle);
	TestTrue("Request 0 and 1 are different objects", bDifferentObjects);
	TestEqual("OldestRequest", OldestRequest, Request0);
	TestEqual("OldestPendingRequest", OldestPendingRequest, (UOUURequest*)nullptr);
	TestEqual("OldestIdleRequest", OldestIdleRequest, Request0);
	TestArraysEqual(*this, "PendingRequests", OpenRequests, ExpectedRequestList);
	
	return true;
}

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(RaiseNewRequest, DEFAULT_OUU_TEST_FLAGS)
{
	// Arrange
	UOUURequestQueue* RequestQueue = NewObject<UOUURequestQueue>();
	RequestQueue->RequestClass = UOUURequestQueue_TestRequest::StaticClass();

	// Act
	UOUURequest* Request0 = RequestQueue->RaiseNewRequest();
	UOUURequest* Request1 = RequestQueue->RaiseNewRequest();
	TArray<UOUURequest*> OpenRequests = RequestQueue->GetRequestsInQueue();
	UOUURequest* OldestRequest = RequestQueue->GetOldestRequest();
	UOUURequest* OldestPendingRequest = RequestQueue->GetOldestRequestWithState(EOUURequestState::Pending);
	UOUURequest* OldestIdleRequest = RequestQueue->GetOldestRequestWithState(EOUURequestState::Idle);

	// Assert
	bool bRequest0CorrectClass = IsValid(Cast<UOUURequestQueue_TestRequest>(Request0));
	bool bRequest1CorrectClass = IsValid(Cast<UOUURequestQueue_TestRequest>(Request1));
	bool bDifferentObjects = Request0 != Request1;
	TArray<UOUURequest*> ExpectedRequestList = { Request0, Request1 };

	TestTrue("Request 0 has correct class", bRequest0CorrectClass);
	TestTrue("Request 1 has correct class", bRequest1CorrectClass);
	TestEqual("Request 0 state", Request0->GetState(), EOUURequestState::Pending);
	TestEqual("Request 1 state", Request1->GetState(), EOUURequestState::Pending);
	TestTrue("Request 0 and 1 are different objects", bDifferentObjects);
	TestEqual("OldestRequest", OldestRequest, Request0);
	TestEqual("OldestPendingRequest", OldestPendingRequest, Request0);
	TestEqual("OldestIdleRequest", OldestIdleRequest, (UOUURequest*)nullptr);
	TestArraysEqual(*this, "PendingRequests", OpenRequests, ExpectedRequestList);

	return true;
}

//////////////////////////////////////////////////////////////////////////

/**
 * 0: action for request 0
 * 1: action for request 1
 * 2: reverse actions?
 *		true - perform action on request 1 first, then on action 0
 *		false - perform action on request 0 first, then on action 1;
 */
OUU_IMPLEMENT_COMPLEX_AUTOMATION_TEST_BEGIN(RaiseRequest_Complete, DEFAULT_OUU_TEST_FLAGS)
OUU_COMPLEX_AUTOMATION_TESTCASE("0|0|0")
OUU_COMPLEX_AUTOMATION_TESTCASE("1|0|0")
OUU_COMPLEX_AUTOMATION_TESTCASE("0|1|0")
OUU_COMPLEX_AUTOMATION_TESTCASE("1|1|0")
// --
OUU_COMPLEX_AUTOMATION_TESTCASE("2|0|0")
OUU_COMPLEX_AUTOMATION_TESTCASE("2|1|0")
OUU_COMPLEX_AUTOMATION_TESTCASE("0|2|0")
OUU_COMPLEX_AUTOMATION_TESTCASE("1|2|0")
OUU_COMPLEX_AUTOMATION_TESTCASE("2|2|0")
// --
OUU_COMPLEX_AUTOMATION_TESTCASE("3|0|0")
OUU_COMPLEX_AUTOMATION_TESTCASE("3|1|0")
OUU_COMPLEX_AUTOMATION_TESTCASE("3|2|0")
OUU_COMPLEX_AUTOMATION_TESTCASE("0|3|0")
OUU_COMPLEX_AUTOMATION_TESTCASE("1|3|0")
OUU_COMPLEX_AUTOMATION_TESTCASE("2|3|0")
OUU_COMPLEX_AUTOMATION_TESTCASE("3|3|0")
// ----
OUU_COMPLEX_AUTOMATION_TESTCASE("0|0|1")
OUU_COMPLEX_AUTOMATION_TESTCASE("1|0|1")
OUU_COMPLEX_AUTOMATION_TESTCASE("0|1|1")
OUU_COMPLEX_AUTOMATION_TESTCASE("1|1|1")
// --
OUU_COMPLEX_AUTOMATION_TESTCASE("2|0|1")
OUU_COMPLEX_AUTOMATION_TESTCASE("2|1|1")
OUU_COMPLEX_AUTOMATION_TESTCASE("0|2|1")
OUU_COMPLEX_AUTOMATION_TESTCASE("1|2|1")
OUU_COMPLEX_AUTOMATION_TESTCASE("2|2|1")
// --
OUU_COMPLEX_AUTOMATION_TESTCASE("3|0|1")
OUU_COMPLEX_AUTOMATION_TESTCASE("3|1|1")
OUU_COMPLEX_AUTOMATION_TESTCASE("3|2|1")
OUU_COMPLEX_AUTOMATION_TESTCASE("0|3|1")
OUU_COMPLEX_AUTOMATION_TESTCASE("1|3|1")
OUU_COMPLEX_AUTOMATION_TESTCASE("2|3|1")
OUU_COMPLEX_AUTOMATION_TESTCASE("3|3|1")
OUU_IMPLEMENT_COMPLEX_AUTOMATION_TEST_END(RaiseRequest_Complete)
{
	enum class ETestRequestAction
	{
		DoNothing = 0,
		Success = 1,
		Fail = 2,
		Cancel = 3
	};

	// Arrange
	FAutomationTestParameterParser Parser{ Parameters };
	const ETestRequestAction Action0 = Parser.GetValue<ETestRequestAction>(0);
	const ETestRequestAction Action1 = Parser.GetValue<ETestRequestAction>(1);
	const bool bReverseActions = Parser.GetValue<bool>(2);

	UOUURequestQueue* RequestQueue = NewObject<UOUURequestQueue>();
	UOUURequest* Request0 = RequestQueue->RaiseNewRequest();
	Request0->bResetAfterCompletion = false;
	UOUURequest* Request1 = RequestQueue->RaiseNewRequest();
	Request1->bResetAfterCompletion = false;

	// Act
	auto PerformTestAction = [](UOUURequest* Request, ETestRequestAction Action) {
		switch (Action) {
		case ETestRequestAction::DoNothing:
			// do nothing
			break;
		case ETestRequestAction::Success:
			Request->Complete(true);
			break;
		case ETestRequestAction::Fail:
			Request->Complete(false);
			break;
		case ETestRequestAction::Cancel:
			Request->Cancel();
			break;
		}
	};

	if (bReverseActions)
	{
		PerformTestAction(Request1, Action1);
		PerformTestAction(Request0, Action0);
	}
	else
	{
		PerformTestAction(Request0, Action0);
		PerformTestAction(Request1, Action1);
	}
	
	// Assert
	auto GetExpectedState = [](ETestRequestAction Action) -> EOUURequestState {
		switch (Action) {
		case ETestRequestAction::Success: return EOUURequestState::Successful;
		case ETestRequestAction::Fail: return EOUURequestState::Failed;
		case ETestRequestAction::Cancel: return EOUURequestState::Canceled;
		}
		return EOUURequestState::Pending;
	};
	TestEqual("Request 0 state", Request0->GetState(), GetExpectedState(Action0));
	TestEqual("Request 1 state", Request1->GetState(), GetExpectedState(Action1));

	TArray<UOUURequest*> ExpectedRequestList;
	if (Action0 == ETestRequestAction::DoNothing)
	{ 
		ExpectedRequestList.Add(Request0); 
	}
	if (Action1 == ETestRequestAction::DoNothing) 
	{ 
		ExpectedRequestList.Add(Request1);
	}
	TArray<UOUURequest*> OpenRequests = RequestQueue->GetRequestsInQueue();
	TestArraysEqual(*this, "PendingRequests", OpenRequests, ExpectedRequestList);

	return true;
}

//////////////////////////////////////////////////////////////////////////

#undef OUU_TEST_CATEGORY
#undef OUU_TEST_TYPE

#endif
