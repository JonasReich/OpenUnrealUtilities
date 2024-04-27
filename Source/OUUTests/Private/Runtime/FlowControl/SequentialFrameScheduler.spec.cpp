// Copyright (c) 2023 Jonas Reich & Contributors

#include "OUUTestUtilities.h"

#if WITH_AUTOMATION_WORKER

	#include "SequentialFrameScheduler/SequentialFrameScheduler.h"

class FTestTaskTarget : public TSharedFromThis<FTestTaskTarget>
{
public:
	int32 TickCount = 0;

	void Tick() { TickCount++; }
};

BEGIN_DEFINE_SPEC(
	FSequentialFrameSchedulerSpec,
	"OpenUnrealUtilities.Runtime.FlowControl.SequentialFrameScheduler",
	DEFAULT_OUU_TEST_FLAGS)
	TSharedPtr<FSequentialFrameScheduler> Scheduler;
	TSharedPtr<FTestTaskTarget> TargetObjectOne;
	TSharedPtr<FTestTaskTarget> TargetObjectTwo;
END_DEFINE_SPEC(FSequentialFrameSchedulerSpec)

void FSequentialFrameSchedulerSpec::Define()
{
	BeforeEach([this]() {
		Scheduler = MakeShared<FSequentialFrameScheduler>();
		TargetObjectOne = MakeShared<FTestTaskTarget>();
		TargetObjectTwo = MakeShared<FTestTaskTarget>();
	});

	Describe("AddTask", [this]() {
		It("should add a task to the queue so it can be executed with the next Tick", [this]() {
			const FSequentialFrameScheduler::FTaskDelegate Delegate =
				FSequentialFrameScheduler::FTaskDelegate::CreateSP(TargetObjectOne.Get(), &FTestTaskTarget::Tick);
			Scheduler->AddTask(Delegate, 1.f);
			Scheduler->Tick(3.f);
			SPEC_TEST_EQUAL(TargetObjectOne->TickCount, 1);
		});
	});

	Describe("RemoveTask", [this]() {
		It("should remove the task again so it won't be ticked anymore", [this]() {
			const FSequentialFrameScheduler::FTaskDelegate Delegate =
				FSequentialFrameScheduler::FTaskDelegate::CreateSP(TargetObjectOne.Get(), &FTestTaskTarget::Tick);
			const auto Handle = Scheduler->AddTask(Delegate, 1.f);
			Scheduler->Tick(3.f);
			Scheduler->RemoveTask(Handle);
			Scheduler->Tick(3.f);
			SPEC_TEST_EQUAL(TargetObjectOne->TickCount, 1);
		});

		It("should not disturb the execution of any other registered tasks", [this]() {
			Scheduler->MaxNumTasksToExecutePerFrame = 2;
			const FSequentialFrameScheduler::FTaskDelegate Delegate =
				FSequentialFrameScheduler::FTaskDelegate::CreateSP(TargetObjectOne.Get(), &FTestTaskTarget::Tick);
			const auto Handle = Scheduler->AddTask(Delegate, 1.f);
			const FSequentialFrameScheduler::FTaskDelegate DelegateTwo =
				FSequentialFrameScheduler::FTaskDelegate::CreateSP(TargetObjectTwo.Get(), &FTestTaskTarget::Tick);
			Scheduler->AddTask(DelegateTwo, 5.f);

			Scheduler->Tick(3.f);
			Scheduler->RemoveTask(Handle);
			Scheduler->Tick(3.f);

			SPEC_TEST_EQUAL(TargetObjectOne->TickCount, 1);
			SPEC_TEST_EQUAL(TargetObjectTwo->TickCount, 2);
		});
	});

	Describe("Tick", [this]() {
		It("should call the delegate even if the timer did not expire if bTickAsOftenAsPossible = true", [this]() {
			const FSequentialFrameScheduler::FTaskDelegate Delegate =
				FSequentialFrameScheduler::FTaskDelegate::CreateSP(TargetObjectOne.Get(), &FTestTaskTarget::Tick);
			constexpr bool bTickAsOftenAsPossible = true;
			Scheduler->AddTask(Delegate, 3.f, bTickAsOftenAsPossible);
			Scheduler->Tick(1.f);
			SPEC_TEST_EQUAL(TargetObjectOne->TickCount, 1);
		});

		It("should NOT call the delegate again if the timer did not expire and bTickAsOftenAsPossible = false",
		   [this]() {
			   const FSequentialFrameScheduler::FTaskDelegate Delegate =
				   FSequentialFrameScheduler::FTaskDelegate::CreateSP(TargetObjectOne.Get(), &FTestTaskTarget::Tick);
			   constexpr bool bTickAsOftenAsPossible = false;
			   Scheduler->AddTask(Delegate, 3.f, bTickAsOftenAsPossible);
			   Scheduler->Tick(1.f);
			   Scheduler->Tick(1.f);
			   SPEC_TEST_EQUAL(TargetObjectOne->TickCount, 1);
		   });

		It("should only call a single task delegate if MaxNumTasksToExecutePerFrame = 1", [this]() {
			Scheduler->MaxNumTasksToExecutePerFrame = 1;
			const FSequentialFrameScheduler::FTaskDelegate Delegate =
				FSequentialFrameScheduler::FTaskDelegate::CreateSP(TargetObjectOne.Get(), &FTestTaskTarget::Tick);
			Scheduler->AddTask(Delegate, 1.f);
			Scheduler->AddTask(Delegate, 1.f);
			Scheduler->Tick(3.f);
			SPEC_TEST_EQUAL(TargetObjectOne->TickCount, 1);
		});

		It("should call two task delegates if MaxNumTasksToExecutePerFrame = 2", [this]() {
			Scheduler->MaxNumTasksToExecutePerFrame = 2;
			const FSequentialFrameScheduler::FTaskDelegate Delegate =
				FSequentialFrameScheduler::FTaskDelegate::CreateSP(TargetObjectOne.Get(), &FTestTaskTarget::Tick);
			Scheduler->AddTask(Delegate, 1.f);
			Scheduler->AddTask(Delegate, 1.f);
			Scheduler->Tick(3.f);
			SPEC_TEST_EQUAL(TargetObjectOne->TickCount, 2);
		});

		It("should not call a task delegate twice in the same frame even if MaxNumTasksToExecutePerFrame = 2",
		   [this]() {
			   Scheduler->MaxNumTasksToExecutePerFrame = 2;
			   const FSequentialFrameScheduler::FTaskDelegate Delegate =
				   FSequentialFrameScheduler::FTaskDelegate::CreateSP(TargetObjectOne.Get(), &FTestTaskTarget::Tick);
			   Scheduler->AddTask(Delegate, 1.f);
			   Scheduler->Tick(3.f);
			   SPEC_TEST_EQUAL(TargetObjectOne->TickCount, 1);
		   });

		It("should call a single task multiple frames in a row if there is no other task requiring ticking, even "
		   "though the period did not expire yet",
		   [this]() {
			   const FSequentialFrameScheduler::FTaskDelegate Delegate =
				   FSequentialFrameScheduler::FTaskDelegate::CreateSP(TargetObjectOne.Get(), &FTestTaskTarget::Tick);
			   Scheduler->AddTask(Delegate, 4.f);
			   Scheduler->Tick(1.f);
			   Scheduler->Tick(1.f);
			   Scheduler->Tick(1.f);
			   SPEC_TEST_EQUAL(TargetObjectOne->TickCount, 3);
		   });

		It("should call two tasks with the same delay alternating one after another in non-deterministic order",
		   [this]() {
			   Scheduler->MaxNumTasksToExecutePerFrame = 1;
			   const FSequentialFrameScheduler::FTaskDelegate DelegateOne =
				   FSequentialFrameScheduler::FTaskDelegate::CreateSP(TargetObjectOne.Get(), &FTestTaskTarget::Tick);
			   Scheduler->AddTask(DelegateOne, 4.f);
			   const FSequentialFrameScheduler::FTaskDelegate DelegateTwo =
				   FSequentialFrameScheduler::FTaskDelegate::CreateSP(TargetObjectTwo.Get(), &FTestTaskTarget::Tick);
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

		It("should fill up tick gaps with calls to tasks that may be ticked every frame while waiting for tasks that "
		   "may not",
		   [this]() {
			   Scheduler->MaxNumTasksToExecutePerFrame = 1;
			   // Task #1 will have a higher tick period, but allows ticking every frame.
			   // It should fill in the gaps even though it has a higher period.
			   const FSequentialFrameScheduler::FTaskDelegate DelegateOne =
				   FSequentialFrameScheduler::FTaskDelegate::CreateSP(TargetObjectOne.Get(), &FTestTaskTarget::Tick);
			   Scheduler->AddTask(DelegateOne, 10.f, true);
			   const FSequentialFrameScheduler::FTaskDelegate DelegateTwo =
				   FSequentialFrameScheduler::FTaskDelegate::CreateSP(TargetObjectTwo.Get(), &FTestTaskTarget::Tick);
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

		It("should support ticking multiple tasks that have a period of 0", [this]() {
			const FSequentialFrameScheduler::FTaskDelegate DelegateOne =
				FSequentialFrameScheduler::FTaskDelegate::CreateSP(TargetObjectOne.Get(), &FTestTaskTarget::Tick);
			Scheduler->AddTask(DelegateOne, 0.f, true);
			const FSequentialFrameScheduler::FTaskDelegate DelegateTwo =
				FSequentialFrameScheduler::FTaskDelegate::CreateSP(TargetObjectTwo.Get(), &FTestTaskTarget::Tick);
			Scheduler->AddTask(DelegateTwo, 0.f, true);

			// Tick 4 times, which means both tasks should tick 2 times each
			Scheduler->Tick(1.f);
			Scheduler->Tick(1.f);
			Scheduler->Tick(1.f);
			Scheduler->Tick(1.f);
			SPEC_TEST_EQUAL(TargetObjectOne->TickCount, 2);
			SPEC_TEST_EQUAL(TargetObjectTwo->TickCount, 2);
		});

		It("should skip ticks of objects that became invalid", [this]() {
			// Make sure the delegates are created in nested scope, so there is no chance we accidentally keep objects
			// valid
			{
				const FSequentialFrameScheduler::FTaskDelegate DelegateOne =
					FSequentialFrameScheduler::FTaskDelegate::CreateSP(TargetObjectOne.Get(), &FTestTaskTarget::Tick);
				Scheduler->AddTask(DelegateOne, 1.f, true);
				const FSequentialFrameScheduler::FTaskDelegate DelegateTwo =
					FSequentialFrameScheduler::FTaskDelegate::CreateSP(TargetObjectTwo.Get(), &FTestTaskTarget::Tick);
				Scheduler->AddTask(DelegateTwo, 1.f, true);
			}

			// Tick 4 times, which means both tasks should tick 2 times each
			Scheduler->Tick(1.f);
			Scheduler->Tick(1.f);

			SPEC_TEST_EQUAL(TargetObjectOne->TickCount, 1);
			SPEC_TEST_EQUAL(TargetObjectTwo->TickCount, 1);

			const TWeakPtr<FTestTaskTarget> WeakRefToTargetObjectOne = TargetObjectOne;
			// Reset object without clearing task, which should invalidate the delegate handle,
			// but not keep the object alive.
			TargetObjectOne.Reset();

			AddExpectedError(TEXT("was auto-removed"));
			SPEC_TEST_FALSE(WeakRefToTargetObjectOne.IsValid());

			// These ticks should now both go to TargetObjectTwo
			Scheduler->Tick(1.f);
			Scheduler->Tick(1.f);

			SPEC_TEST_EQUAL(TargetObjectTwo->TickCount, 3);
		});
	});
}

#endif
