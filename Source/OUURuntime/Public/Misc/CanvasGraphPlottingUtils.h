// Copyright (c) 2022 Jonas Reich

#pragma once

#include "CoreMinimal.h"

class FCanvas;

namespace CanvasGraphPlottingUtils
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
				TFunctionRef<float(const void*, int32)> InGetValueDelegate,
				TFunctionRef<float(const void*)> InGetNumDelegate) :
				ValueContainer(InValueContainer),
				GetValueDelegate(InGetValueDelegate),
				GetNumDelegate(InGetNumDelegate){};

			const void* ValueContainer = nullptr;
			TFunctionRef<float(const void*, int32)> GetValueDelegate;
			TFunctionRef<float(const void*)> GetNumDelegate;

			float operator[](int32 Idx) const { return GetValueDelegate(ValueContainer, Idx); }
			int32 Num() const { return GetNumDelegate(ValueContainer); }
		};

		FGraphStatData(FValueRangeRef InValueRangeRef, FLinearColor InColor, FString InTitle) :
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
		const FString GraphTitle,
		float HighestValue,
		bool bUseLogarithmicYAxis = false);
} // namespace CanvasGraphPlottingUtils
