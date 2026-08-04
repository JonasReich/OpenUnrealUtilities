
#pragma once
#include "CoreMinimal.h"

#include "ChartCreation.h"
#include "HAL/PlatformMemoryHelpers.h"
#include "Templates/CircularAggregator.h"

namespace OUU::Runtime
{
	enum class ETimingChannel
	{
		Frame,
		Game,
		Render,
		RHI,
		GPU,
		FlushAsyncLoading,
		Num
	};

	// Aggregate data for a single channel
	struct FChannelTimingResult
	{
	public:
		double Avg = 0.0;
		double Min = 0.0;
		double Max = 0.0;
		// the n-th percentile time spent on frames in this timing channel
		// (n = TPerformanceAggregator::TimingPercentile, default: 95th percentile)
		double Percentile = 0.0;
	};

	// Snapshot returned once per reporting interval.
	struct FAggregatePerformanceReport
	{
		// How many frame samples were taken to compile this report.
		int32 FrameCount = 0;

		// --- TIMINGS ---
		double TotalSeconds = 0.0;

		// Per channel timing info
		FChannelTimingResult ChannelTimingsSeconds[static_cast<int32>(ETimingChannel::Num)] = {};

		// How many total frames count as hitches
		int32 TotalHitchFrames = 0;
		// Most common type of hitch
		EFrameHitchType MostCommonHitchType = EFrameHitchType::NoHitch;
		// Biggest base load
		ETimingChannel MostCongestedChannel = ETimingChannel::Num;

		// --- MEMORY ---
		double AveragePhysicalMemory_Used_MiB = 0.0;
		double AverageVirtualMemory_Used_MiB = 0.0;
		double AveragePhysicalMemory_Free_MiB = 0.0;

		// --- RENDERING ---
		double AverageDrawCalls = 0.0;
		double AveragePrimitivesDrawn = 0.0;

		// --- OTHER ---
		int32 TotalLoadCalls = 0;
	};

	// Aggregate performance values over a flexible time period, but at max MaxSamples frames.
	// Because of this MaxSamples setting you can safely let this run in the background without having to worry about
	// leaking memory, at the cost of potentially losing data on the far side of the capture.
	template <int32 MaxSamples>
	class TPerformanceAggregator :
		public IPerformanceDataConsumer,
		public TSharedFromThis<TPerformanceAggregator<MaxSamples>>
	{
	public:
		// Call on your reporting cadence to read + reset the cached data.
		// Note for child classes: Read from members of this base class before calling super - otherwise they will
		// already be cleared. Make sure to clear any members in child classes to reset tracking for the next interval!
		FAggregatePerformanceReport AggregateData()
		{
			// Create a copy of the report that can be returned at the end of the function, even though we reset the
			// member state.
			FAggregatePerformanceReport R = CopyTemp(CurrentIntervalReport);

			// Calculate derived values for the report.
			if (R.FrameCount > 0)
			{
				for (int32 ChannelIdx = 0; ChannelIdx < static_cast<int32>(ETimingChannel::Num); ++ChannelIdx)
				{
					auto& SourceChannel = TimingChannelsSeconds[ChannelIdx];
					auto SortedCopy = TArray<double>(SourceChannel.UnsortedValues());
					SortedCopy.Sort();
					const int32 N = SortedCopy.Num();
					double Sum = 0.0;
					for (float V : SortedCopy)
					{
						Sum += V;
					}

					R.ChannelTimingsSeconds[ChannelIdx].Avg = static_cast<float>(Sum / N);
					R.ChannelTimingsSeconds[ChannelIdx].Min = SortedCopy[0];
					R.ChannelTimingsSeconds[ChannelIdx].Max = SortedCopy[N - 1];
					R.ChannelTimingsSeconds[ChannelIdx].Percentile = Percentile(SortedCopy, TimingPercentile / 100.0);
				}

				R.MostCommonHitchType = EFrameHitchType::NoHitch;
				int32 MaxHitchCount = 0;
				// Skip the  EFrameHitchType::NoHitch (0) case
				for (int32 HitchType = (int32)EFrameHitchType::UnknownUnit; HitchType <= (int32)EFrameHitchType::GPU;
					 ++HitchType)
				{
					if (HitchCounts[HitchType] > MaxHitchCount)
					{
						R.MostCommonHitchType = static_cast<EFrameHitchType>(HitchType);
						MaxHitchCount = HitchCounts[HitchType];
					}
				}

				// frame is the default, as in "the whole frame is long, but not a single channel"
				R.MostCongestedChannel = ETimingChannel::Frame;
				int32 MaxBoundChannelCount = 0;
				// Skip the frame summary channel
				for (int32 ChannelIdx = (int32)ETimingChannel::Frame + 1; ChannelIdx < (int32)ETimingChannel::Num;
					 ++ChannelIdx)
				{
					if (BoundCounts[ChannelIdx] > MaxBoundChannelCount)
					{
						R.MostCongestedChannel = static_cast<ETimingChannel>(ChannelIdx);
						MaxBoundChannelCount = BoundCounts[ChannelIdx];
					}
				}

				// --- MEMORY ---
				// average usage per frame. total sum is meaningless (max could be, but for the most part we care about
				// avg).
				R.AveragePhysicalMemory_Used_MiB = TotalPhysicalMemory_Used_MiB / R.FrameCount;
				R.AverageVirtualMemory_Used_MiB = TotalVirtualMemory_Used_MiB / R.FrameCount;
				R.AveragePhysicalMemory_Free_MiB = TotalPhysicalMemory_Free_MiB / R.FrameCount;

				// --- RENDERING ---
				// average render load per frame. total count is meaningless.
				R.AverageDrawCalls = TotalDrawCalls / R.FrameCount;
				R.AveragePrimitivesDrawn = TotalPrimitivesDrawn / R.FrameCount;

				// --- OTHER ---
				// do not average these. compared to draw calls, we don't expect loads every frame, so comparing the
				// absolute number of these to absolute number of hitches could be insightful.
				R.TotalLoadCalls = TotalLoadCalls;
			}

			// Reset members (report + auxiliary members)
			CurrentIntervalReport = {};

			// It's fine to reset all the channel values. Hitches are detected by UE code, so we just care about the
			// average in the last interval, not a rolling average.
			// #TODO could this reset be made cheaper, e.g. by explicitly clearing memory?
			std::ranges::fill(TimingChannelsSeconds, TFixedSizeCircularAggregator<double, MaxSamples>{});
			std::ranges::fill(HitchCounts, 0);
			std::ranges::fill(BoundCounts, 0);

			TotalPhysicalMemory_Free_MiB = 0.0;
			TotalVirtualMemory_Used_MiB = 0.0;
			TotalPhysicalMemory_Used_MiB = 0.0;
			TotalDrawCalls = 0.0;
			TotalPrimitivesDrawn = 0.0;
			TotalLoadCalls = 0.0;

			return R;
		}

		// -- IPerformanceDataConsumer interface
	public:
		void StartCharting() override {}

		void ProcessFrame(const FFrameData& FrameData) override
		{
			++CurrentIntervalReport.FrameCount;

			// Either includes or excludes idle time, depending on t.FPSChart.ExcludeIdleTime, so we need to
			// compensate for that.
			double TotalFrame = FPerformanceTrackingSystem::ShouldExcludeIdleTimeFromCharts()
				? FrameData.DeltaSeconds
				: FrameData.DeltaSeconds - FrameData.IdleSeconds;

			CurrentIntervalReport.TotalSeconds += TotalFrame;

			// All the Epic values are in seconds, so we retain that scale for as long as possible.
			// Consumers may want to convert to milliseconds values.
			TimingChannelsSeconds[static_cast<int32>(ETimingChannel::Frame)].Add(TotalFrame);
			TimingChannelsSeconds[static_cast<int32>(ETimingChannel::Game)].Add(FrameData.GameThreadTimeSeconds);
			TimingChannelsSeconds[static_cast<int32>(ETimingChannel::Render)].Add(FrameData.RenderThreadTimeSeconds);
			TimingChannelsSeconds[static_cast<int32>(ETimingChannel::RHI)].Add(FrameData.RHIThreadTimeSeconds);
			TimingChannelsSeconds[static_cast<int32>(ETimingChannel::GPU)].Add(FrameData.GPUTimeSeconds);
			TimingChannelsSeconds[static_cast<int32>(ETimingChannel::FlushAsyncLoading)].Add(
				FrameData.FlushAsyncLoadingTime);

			// Record bound counts
			if (FrameData.bGameThreadBound)
			{
				++BoundCounts[static_cast<int32>(ETimingChannel::Game)];
			}
			else if (FrameData.bRenderThreadBound)
			{
				++BoundCounts[static_cast<int32>(ETimingChannel::Render)];
			}
			else if (FrameData.bRHIThreadBound)
			{
				++BoundCounts[static_cast<int32>(ETimingChannel::RHI)];
			}
			else if (FrameData.bGPUBound)
			{
				++BoundCounts[static_cast<int32>(ETimingChannel::GPU)];
			}

			// Rely on FrameData hitch detection to match engine settings, instead of rolling our own.
			// Hitch thresholds can be configured via cvars (e.g. t.HitchFrameTimeThreshold)
			++HitchCounts[static_cast<int32>(FrameData.HitchStatus)];
			if (FrameData.HitchStatus != EFrameHitchType::NoHitch)
			{
				++CurrentIntervalReport.TotalHitchFrames;
			}

			// Memory
			FPlatformMemoryStats MemoryStats = PlatformMemoryHelpers::GetFrameMemoryStats();
			TotalVirtualMemory_Used_MiB += (MemoryStats.UsedVirtual / (1024 * 1024));
			TotalPhysicalMemory_Used_MiB += (MemoryStats.UsedPhysical / (1024 * 1024));
			TotalPhysicalMemory_Free_MiB += (MemoryStats.AvailablePhysical / (1024 * 1024));
			TotalPrimitivesDrawn += GNumPrimitivesDrawnRHI[0];
			TotalDrawCalls += GNumDrawCallsRHI[0];

			TotalLoadCalls += FrameData.SyncLoadCount;
			TotalLoadCalls += FrameData.FlushAsyncLoadingCount;
		}

		void StopCharting() override {}

	private:
		// Assumes Sorted is ascending and non-empty. P in [0,1].
		static float Percentile(const TArray<double>& Sorted, double Percentile)
		{
			const int32 Num = Sorted.Num();
			if (Num == 1)
			{
				return Sorted[0];
			}
			const float Rank = Percentile * (Num - 1);
			const int32 Lo = FMath::FloorToInt(Rank);
			const int32 Hi = FMath::Min(Lo + 1, Num - 1);
			return FMath::Lerp(Sorted[Lo], Sorted[Hi], Rank - Lo);
		}

	public:
		// Which nth-percentile to report in the FChannelTimingResult values computed from TimingChannels data below
		int32 TimingPercentile = 95;

	private:
		FAggregatePerformanceReport CurrentIntervalReport;

		// Count how often each hitch type occured
		// -> We don't have any overflow protection for this, which could cause issues if the aggregator isn't flushed.
		static_assert(static_cast<int32>(EFrameHitchType::GPU) == 5, "hitch types changed");
		int32 HitchCounts[6] = {};

		// Count how often we were bound by the respective channel.
		// The summary can output a single value, which thread is the biggest limit for this player.
		// This can't be linked back to the timing channel values easily, but we only really use it to determine the
		// "most bound" channel, so this should be fine.
		int32 BoundCounts[static_cast<int32>(ETimingChannel::Num)] = {};

		// Track individual frame times per channel. Note that these are capped at MaxSamples, so any data before that
		// will get lost even though other metrics (like hitch counts) persist!
		TFixedSizeCircularAggregator<double, MaxSamples> TimingChannelsSeconds[static_cast<int32>(ETimingChannel::Num)];

		// For these stats we just track the total sum to compute averages.
		// Use double instead of int to rule out overflow errors. Not as accurate, but should still bring us in the
		// right ballpark when dividing by frame count at the end of an interval.
		double TotalPhysicalMemory_Used_MiB = 0.0;
		double TotalVirtualMemory_Used_MiB = 0.0;
		double TotalPhysicalMemory_Free_MiB = 0.0;
		double TotalDrawCalls = 0.0;
		double TotalPrimitivesDrawn = 0.0;
		double TotalLoadCalls = 0.0;
	};
} // namespace OUU::Runtime
