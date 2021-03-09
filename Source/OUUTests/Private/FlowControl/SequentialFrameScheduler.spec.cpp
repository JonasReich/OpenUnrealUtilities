// Copyright (c) 2021 Jonas Reich

#include "OUUTestUtilities.h"

#if WITH_AUTOMATION_WORKER

#include "SequentialFrameScheduler/SequentialFrameScheduler.h"

class FTestTaskTarget
{
public:
	int32 TickCount = 0;

	void Tick()
	{
		TickCount++;
	}
};

BEGIN_DEFINE_SPEC(FSequentialFrameSchedulerSpec, "OpenUnrealUtilities.FlowControl.SequentialFrameScheduler", DEFAULT_OUU_TEST_FLAGS)
	TSharedPtr<FSequentialFrameScheduler> Scheduler;
	TSharedPtr<FTestTaskTarget> TargetObjectOne;
	TSharedPtr<FTestTaskTarget> TargetObjectTwo;
END_DEFINE_SPEC(FSequentialFrameSchedulerSpec)

void FSequentialFrameSchedulerSpec::Define()
{
	BeforeEach([this]()
	{
		Scheduler = MakeShared<FSequentialFrameScheduler>();
		TargetObjectOne = MakeShared<FTestTaskTarget>();
		TargetObjectTwo = MakeShared<FTestTaskTarget>();
	});

	Describe("AddTask", [this]()
	{
		It("should add a task to the queue so it can be executed with the next Tick", [this]()
		{
			FSequentialFrameScheduler::FTaskDelegate Delegate = FSequentialFrameScheduler::FTaskDelegate::CreateRaw(TargetObjectOne.Get(), &FTestTaskTarget::Tick);
			Scheduler->AddTask(Delegate, 1.f);
			Scheduler->Tick(3.f);
			SPEC_TEST_EQUAL(TargetObjectOne->TickCount, 1);
		});
	});

	Describe("RemoveTask", [this]()
	{
		It("should remove the task again so it won't be ticked anymore", [this]()
		{
			FSequentialFrameScheduler::FTaskDelegate Delegate = FSequentialFrameScheduler::FTaskDelegate::CreateRaw(TargetObjectOne.Get(), &FTestTaskTarget::Tick);
			auto Handle = Scheduler->AddTask(Delegate, 1.f);
			Scheduler->Tick(3.f);
			Scheduler->RemoveTask(Handle);
			Scheduler->Tick(3.f);
			SPEC_TEST_EQUAL(TargetObjectOne->TickCount, 1);
		});

		It("should not disturb the execution of any other registered tasks", [this]()
		{
			FSequentialFrameScheduler::FTaskDelegate Delegate = FSequentialFrameScheduler::FTaskDelegate::CreateRaw(TargetObjectOne.Get(), &FTestTaskTarget::Tick);
			auto Handle = Scheduler->AddTask(Delegate, 1.f);
			FSequentialFrameScheduler::FTaskDelegate DelegateTwo = FSequentialFrameScheduler::FTaskDelegate::CreateRaw(TargetObjectTwo.Get(), &FTestTaskTarget::Tick);
			Scheduler->AddTask(DelegateTwo, 5.f);

			Scheduler->Tick(3.f);
			Scheduler->RemoveTask(Handle);
			Scheduler->Tick(3.f);

			SPEC_TEST_EQUAL(TargetObjectOne->TickCount, 1);
			SPEC_TEST_EQUAL(TargetObjectTwo->TickCount, 2);
		});
	});

	Describe("Tick", [this]()
	{
		It("should call the delegate even if the timer did not expire if bTickAsOftenAsPossible = true", [this]()
		{
			FSequentialFrameScheduler::FTaskDelegate Delegate = FSequentialFrameScheduler::FTaskDelegate::CreateRaw(TargetObjectOne.Get(), &FTestTaskTarget::Tick);
			const bool bTickAsOftenAsPossible = true;
			Scheduler->AddTask(Delegate, 3.f, bTickAsOftenAsPossible);
			Scheduler->Tick(1.f);
			SPEC_TEST_EQUAL(TargetObjectOne->TickCount, 1);
		});

		It("should NOT call the delegate again if the timer did not expire and bTickAsOftenAsPossible = false", [this]()
		{
			FSequentialFrameScheduler::FTaskDelegate Delegate = FSequentialFrameScheduler::FTaskDelegate::CreateRaw(TargetObjectOne.Get(), &FTestTaskTarget::Tick);
			const bool bTickAsOftenAsPossible = false;
			Scheduler->AddTask(Delegate, 3.f, bTickAsOftenAsPossible);
			Scheduler->Tick(1.f);
			Scheduler->Tick(1.f);
			SPEC_TEST_EQUAL(TargetObjectOne->TickCount, 1);
		});

		It("should only call a single task delegate if MaxNumTasksToExecutePerFrame = 1", [this]()
		{
			Scheduler->MaxNumTasksToExecutePerFrame = 1;
			FSequentialFrameScheduler::FTaskDelegate Delegate = FSequentialFrameScheduler::FTaskDelegate::CreateRaw(TargetObjectOne.Get(), &FTestTaskTarget::Tick);
			Scheduler->AddTask(Delegate, 1.f);
			Scheduler->AddTask(Delegate, 1.f);
			Scheduler->Tick(3.f);
			SPEC_TEST_EQUAL(TargetObjectOne->TickCount, 1);
		});

		It("should call two task delegates if MaxNumTasksToExecutePerFrame = 2", [this]()
		{
			Scheduler->MaxNumTasksToExecutePerFrame = 2;
			FSequentialFrameScheduler::FTaskDelegate Delegate = FSequentialFrameScheduler::FTaskDelegate::CreateRaw(TargetObjectOne.Get(), &FTestTaskTarget::Tick);
			Scheduler->AddTask(Delegate, 1.f);
			Scheduler->AddTask(Delegate, 1.f);
			Scheduler->Tick(3.f);
			SPEC_TEST_EQUAL(TargetObjectOne->TickCount, 2);
		});

		It("should not call a task delegate twice in the same frame even if MaxNumTasksToExecutePerFrame = 2", [this]()
		{
			Scheduler->MaxNumTasksToExecutePerFrame = 2;
			FSequentialFrameScheduler::FTaskDelegate Delegate = FSequentialFrameScheduler::FTaskDelegate::CreateRaw(TargetObjectOne.Get(), &FTestTaskTarget::Tick);
			Scheduler->AddTask(Delegate, 1.f);
			Scheduler->Tick(3.f);
			SPEC_TEST_EQUAL(TargetObjectOne->TickCount, 1);
		});

		It("should call a single task multiple frames in a row if there is no other task requiring ticking, even though the period did not expire yet", [this]()
		{
			FSequentialFrameScheduler::FTaskDelegate Delegate = FSequentialFrameScheduler::FTaskDelegate::CreateRaw(TargetObjectOne.Get(), &FTestTaskTarget::Tick);
			Scheduler->AddTask(Delegate, 4.f);
			Scheduler->Tick(1.f);
			Scheduler->Tick(1.f);
			Scheduler->Tick(1.f);
			SPEC_TEST_EQUAL(TargetObjectOne->TickCount, 3);
		});

		It("should call two tasks with the same delay alternating one after another in non-deterministic order", [this]()
		{
			Scheduler->MaxNumTasksToExecutePerFrame = 1;
			FSequentialFrameScheduler::FTaskDelegate DelegateOne = FSequentialFrameScheduler::FTaskDelegate::CreateRaw(TargetObjectOne.Get(), &FTestTaskTarget::Tick);
			Scheduler->AddTask(DelegateOne, 4.f);
			FSequentialFrameScheduler::FTaskDelegate DelegateTwo = FSequentialFrameScheduler::FTaskDelegate::CreateRaw(TargetObjectTwo.Get(), &FTestTaskTarget::Tick);
			Scheduler->AddTask(DelegateTwo, 4.f);

			// Because the order is non-deterministic we have to execute and test in pairs,
			// as we cannot be sure on the order of sorting with the same delays
			Scheduler->Tick(1.f);
			Scheduler->Tick(1.f);
			SPEC_TEST_EQUAL(TargetObjectOne->TickCount, 1);
			SPEC_TEST_EQUAL(TargetObjectTwo->TickCount, 1);
			Scheduler->Tick(1.f);
			Scheduler->Tick(1.f);
			SPEC_TEST_EQUAL(TargetObjectOne->TickCount, 2);
			SPEC_TEST_EQUAL(TargetObjectTwo->TickCount, 2);
		});

		It("should fill up tick gaps with calls to tasks that may be ticked every frame while waiting for tasks that may not", [this]()
		{
			Scheduler->MaxNumTasksToExecutePerFrame = 1;
			// Task #1 will have a higher tick period, but allows ticking every frame.
			// It should fill in the gaps even though it has a higher period. 
			FSequentialFrameScheduler::FTaskDelegate DelegateOne = FSequentialFrameScheduler::FTaskDelegate::CreateRaw(TargetObjectOne.Get(), &FTestTaskTarget::Tick);
			Scheduler->AddTask(DelegateOne, 10.f, true);
			FSequentialFrameScheduler::FTaskDelegate DelegateTwo = FSequentialFrameScheduler::FTaskDelegate::CreateRaw(TargetObjectTwo.Get(), &FTestTaskTarget::Tick);
			Scheduler->AddTask(DelegateTwo, 5.f, false);

			// Object #2 should be ticked first.
			// It has a higher fractional delay after 1s because 1s/5s is 2x bigger than 1s/10s
			Scheduler->Tick(1.f);
			SPEC_TEST_EQUAL(TargetObjectOne->TickCount, 0);
			SPEC_TEST_EQUAL(TargetObjectTwo->TickCount, 1);

			// The next ticks should all be filled in by Task #1
			Scheduler->Tick(1.f);
			Scheduler->Tick(1.f);
			Scheduler->Tick(1.f);
			Scheduler->Tick(1.f);
			SPEC_TEST_EQUAL(TargetObjectOne->TickCount, 4);
			SPEC_TEST_EQUAL(TargetObjectTwo->TickCount, 1);

			// After the delay is over again the next tick should go to Task #2 again
			Scheduler->Tick(1.f);
			SPEC_TEST_EQUAL(TargetObjectOne->TickCount, 4);
			SPEC_TEST_EQUAL(TargetObjectTwo->TickCount, 2);
		});
	});
}

#endif
