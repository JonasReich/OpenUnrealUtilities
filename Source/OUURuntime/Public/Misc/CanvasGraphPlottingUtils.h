// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

class FCanvas;

namespace OUU::Runtime::CanvasGraphPlottingUtils
{
	struct OUURUNTIME_API FGraphStatData
	{
	public:
		/**
		 * Somewhat hacky workaround that provides accessors to multiple CircularAggregator types without
		 * inheritance/virtual calls/template specializations/etc.
		 * Should only be used for this one purpose, as soon as we need more, we should refactor more...
		 */
		struct OUURUNTIME_API FValueRangeRef
		{
			FValueRangeRef() = delete;

			template <class T>
			FValueRangeRef(const T& InCircularAggregator) :
				ValueContainer(&InCircularAggregator),
				GetValueDelegate([](const void* ContainerPtr, int32 Idx) -> float {
					return static_cast<const T*>(ContainerPtr)->operator[](Idx);
				}),
				GetNumDelegate(
					[](const void* ContainerPtr) -> int32 { return static_cast<const T*>(ContainerPtr)->Num(); })
			{
			}

			FValueRangeRef(
				const void* InValueContainer,
				const TFunction<float(const void*, int32)>& InGetValueDelegate,
				const TFunction<float(const void*)>& InGetNumDelegate) :
				ValueContainer(InValueContainer),
				GetValueDelegate(InGetValueDelegate),
				GetNumDelegate(InGetNumDelegate){};

			const void* ValueContainer = nullptr;
			TFunction<float(const void*, int32)> GetValueDelegate;
			TFunction<float(const void*)> GetNumDelegate;

			float operator[](int32 Idx) const { return GetValueDelegate(ValueContainer, Idx); }
			int32 Num() const { return GetNumDelegate(ValueContainer); }
		};

		FGraphStatData(const FValueRangeRef& InValueRangeRef, FLinearColor InColor, const FString& InTitle) :
			ValueAggregator(InValueRangeRef), Color(InColor), Title(InTitle)
		{
		}

		FValueRangeRef ValueAggregator;
		FLinearColor Color;
		FString Title;
	};

	void OUURUNTIME_API DrawCanvasGraph(
		FCanvas* InCanvas,
		float GraphLeftXPos,
		float GraphBottomYPos,
		const TArray<FGraphStatData>& StatsToDraw,
		const FString& GraphTitle,
		float HighestValue,
		bool bUseLogarithmicYAxis = false);
} // namespace OUU::Runtime::CanvasGraphPlottingUtils

namespace UE_DEPRECATED(
	5.0,
	"The namespace CanvasGraphPlottingUtils has been deprecated in favor of OUU::Runtime::CanvasGraphPlottingUtils.")
	CanvasGraphPlottingUtils
{
	using OUU::Runtime::CanvasGraphPlottingUtils::DrawCanvasGraph;
	using OUU::Runtime::CanvasGraphPlottingUtils::FGraphStatData;
} // namespace CanvasGraphPlottingUtils
