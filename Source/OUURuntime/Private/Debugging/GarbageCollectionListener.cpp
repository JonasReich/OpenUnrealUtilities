// Copyright (c) 2021 Jonas Reich

#if WITH_EDITOR
#include "Editor.h"
#endif
#include "LogOpenUnrealUtilities.h"
#include "BehaviorTree/BTNode.h"
#include "Components/Widget.h"

static TAutoConsoleVariable<bool> bCVarAccumulateGCCounts(TEXT("ouu.GC.AccumulateDumps"),
	true,
	TEXT("If true GC reports are only logged at the moment dumping is shut off. Otherwise every GC call triggers a log dump"),
	ECVF_Cheat);

// Right now we only track the count, but it would be great to extend this
// with some other metrics that we can still get during GC.
struct FGarbageCollectionStats
{
	int32 Count = 0;
};

using FClassToGCStats = TTuple<TSoftClassPtr<UObject>, FGarbageCollectionStats>;

/**
 * Delete listener that tracks deleted UObjects over time to gather metrics about object deletion and garbage collection.
 */
class FGarbageCollectionListener : public FUObjectArray::FUObjectDeleteListener
{
private:
	static TUniquePtr<FGarbageCollectionListener> GGarbageCollectionListener;

	FTimerHandle TimerHandle;
	TMap<TSoftClassPtr<UObject>, FGarbageCollectionStats> ClassToStatsMap;
	int32 FrameCounter = 0;
	double StartTime = 0;
	double EndTime = 0;

	void Tick()
	{
		FrameCounter++;
		TimerHandle.Invalidate();
		if (bAutoDeactivate)
		{
			Deactivate();
		}
		else
		{
			if (!bCVarAccumulateGCCounts.GetValueOnGameThread())
			{
				DumpCurrentClassDeletions();

				// This is only required for the case of dumping every frame,
				// so we don't do it here instead of inside DumpCurrentClassDeletions()
				ClassToStatsMap.Reset();
				FrameCounter = 0;
				StartTime = FPlatformTime::Seconds();
			}
			LazySetTimerForNextTick();
		}
	}

	void LazySetTimerForNextTick()
	{
		if (TimerHandle.IsValid())
			return;

		if (auto* TimerManager = GetTimerManager())
		{
			const FTimerDelegate Delegate = FTimerDelegate::CreateRaw(this, &FGarbageCollectionListener::Tick);
			TimerHandle = TimerManager->SetTimerForNextTick(Delegate);
		}
	}

	void ClearTimer()
	{
		if (auto* TimerManager = GetTimerManager())
		{
			TimerManager->ClearTimer(TimerHandle);
		}
	}

	static FTimerManager* GetTimerManager()
	{
		if (UWorld* World = GEngine->GetCurrentPlayWorld())
		{
			return &(World->GetTimerManager());
		}
#if WITH_EDITOR
		if (GIsEditor)
		{
			return &*GEditor->GetTimerManager();
		}
#endif
		return nullptr;
	}

	void DumpCurrentClassDeletions()
	{
		if (ClassToStatsMap.Num() == 0)
			return;

		EndTime = FPlatformTime::Seconds();

		// Sorted from specific to generic.
		// Otherwise all objects would be put into a generic category like UObject.
		TArray<UClass*> GroupingSuperClasses =
		{
			UEdGraphNode::StaticClass(),
			UWidget::StaticClass(),
			UBTNode::StaticClass(),
			UActorComponent::StaticClass(),
			AActor::StaticClass(),
			UObject::StaticClass()
		};

		TMap<UClass*, TArray<FClassToGCStats>> SortedClassDeletionMaps;
		TMap<UClass*, FClassToGCStats> AccumulatedDeletionMaps;

		int32 TotalDeletionCount = 0;
		for (auto& Pair : ClassToStatsMap)
		{
			TotalDeletionCount += Pair.Value.Count;

			UClass* SuperClass = UObject::StaticClass();
			if (Pair.Key.IsValid())
			{
				if (auto** SuperClassPtr = GroupingSuperClasses.FindByPredicate([&](UClass* C) -> bool
				{
					return Pair.Key->IsChildOf(C);
				}))
				{
					SuperClass = *SuperClassPtr;
				}
			}

			SortedClassDeletionMaps.FindOrAdd(SuperClass).Add(Pair);
			AccumulatedDeletionMaps.FindOrAdd(SuperClass).Value.Count += Pair.Value.Count;
		}

		GroupingSuperClasses.Sort([&](const UClass& A, const UClass& B) -> bool
		{
			FClassToGCStats* AccumulatedStatsA = AccumulatedDeletionMaps.Find(&A);
			FClassToGCStats* AccumulatedStatsB = AccumulatedDeletionMaps.Find(&B);

			if (AccumulatedStatsA == nullptr || AccumulatedStatsB == nullptr)
				return false;

			return AccumulatedStatsA->Value.Count > AccumulatedStatsB->Value.Count;
		});

		UE_LOG(LogOpenUnrealUtilities, Log, TEXT("--- Garbage Collection Summary ---"));
		const double TimePassed = EndTime - StartTime;
		UE_LOG(LogOpenUnrealUtilities, Log, TEXT("Deleted %i UObjects in %f seconds (%i frames). See breakdown per class below:"),
			TotalDeletionCount, TimePassed, FrameCounter);

		TSoftClassPtr<UObject> Class = nullptr;
		FGarbageCollectionStats Stats;
		for (auto* SuperClass : GroupingSuperClasses)
		{
			TArray<FClassToGCStats>* SortedClassDeletionMap = SortedClassDeletionMaps.Find(SuperClass);
			if (SortedClassDeletionMap == nullptr)
				continue;

			SortedClassDeletionMap->Sort([](const FClassToGCStats& A, const FClassToGCStats& B) -> bool
			{
				// sort descending values
				return A.Value.Count > B.Value.Count;
			});

			UE_LOG(LogOpenUnrealUtilities, Log, TEXT("- %i with super class %s"), AccumulatedDeletionMaps[SuperClass].Value.Count, *SuperClass->GetName());
			for (auto& Pair : (*SortedClassDeletionMap))
			{
				Tie(Class, Stats) = Pair;
				UE_LOG(LogOpenUnrealUtilities, Log, TEXT("\t- %i %s"), Stats.Count, *(Class.IsValid() ? Class->GetName() : FString("invalid class")));
			}
		}
	}

public:
	/** Auto deactivate the listener a frame after the next garbage collection */
	bool bAutoDeactivate = false;

	FGarbageCollectionListener()
	{
		GUObjectArray.AddUObjectDeleteListener(this);
		StartTime = FPlatformTime::Seconds();
	}

	virtual ~FGarbageCollectionListener()
	{
		ClearTimer();
		DumpCurrentClassDeletions();
		GUObjectArray.RemoveUObjectDeleteListener(this);
	}

	static FGarbageCollectionListener& FindOrCreate()
	{
		if (!GGarbageCollectionListener.IsValid())
		{
			GGarbageCollectionListener = MakeUnique<FGarbageCollectionListener>();
		}
		return *GGarbageCollectionListener;
	}

	static FGarbageCollectionListener* Find()
	{
		return GGarbageCollectionListener.Get();
	}

	static void Deactivate()
	{
		GGarbageCollectionListener.Reset();
	}

	// - FUObjectArray::FUObjectDeleteListener
	virtual void NotifyUObjectDeleted(const UObjectBase* ObjectBase, int32 Index) override
	{
		auto& Stats = ClassToStatsMap.FindOrAdd(ObjectBase->GetClass(), {});
		Stats.Count += 1;
		LazySetTimerForNextTick();
	}

	virtual void OnUObjectArrayShutdown() override
	{
		// Target array is shutting down so we can deactivate.
		// Probably won't log anything anymore because this should only happen when the engine shuts down.
		Deactivate();
	}

	// --
};

TUniquePtr<FGarbageCollectionListener> FGarbageCollectionListener::GGarbageCollectionListener;

static FAutoConsoleCommand DumpGarbageCollection(
	TEXT("ouu.GC.DumpNext"),
	TEXT("Dump a summary of the next garbage collection that is triggered into the output log"),
	FConsoleCommandDelegate::CreateStatic(
		[]()
		{
			FGarbageCollectionListener::FindOrCreate().bAutoDeactivate = true;
		})
	);

static FAutoConsoleCommand StartGarbageCollectionDumping(
	TEXT("ouu.GC.StartDump"),
	TEXT("Start dumping summaries of all following garbage collections into the output log until this process is stopped with ouu.GC.StopDump"),
	FConsoleCommandDelegate::CreateStatic(
		[]()
		{
			FGarbageCollectionListener::FindOrCreate().bAutoDeactivate = false;
		})
	);

static FAutoConsoleCommand StopGarbageCollectionDumping(
	TEXT("ouu.GC.StopDump"),
	TEXT("Stop dumping garbage collection summaries into the output log"),
	FConsoleCommandDelegate::CreateStatic(
		[]()
		{
			if (auto* Listener = FGarbageCollectionListener::Find())
			{
				Listener->Deactivate();
			}
		})
	);
