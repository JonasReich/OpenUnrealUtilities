// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "ActorMapWindow/OUUActorMapWindow.h"
#include "Slate/SplitterColumnSizeData.h"
#include "Tickable.h"
#include "Widgets/SWidget.h"
#include "Widgets/Views/SListView.h"

class ASceneCapture2D;

namespace OUU::Developer::ActorMapWindow::Private
{
	extern FText GInvalidText;

	/**
	 * The data and core functionality of the actor map window:
	 * SActorMap takes care of creating objects, widgets and performing actor queries in tick.
	 */
	class SActorMap : public SCompoundWidget
	{
		using Super = SCompoundWidget;

		SLATE_BEGIN_ARGS(SActorMap)
			{
			}
		SLATE_END_ARGS()

		virtual ~SActorMap() override;

		// - SWidget
		void Construct(const FArguments& InArgs);

		virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
			override;
		// --

		/** Separate initializer outside of construct so the widget can be reused for a different world */
		void InitializeForWorld(UWorld* InTargetWorld);
		FORCEINLINE UWorld* GetTargetWorld() const { return TargetWorld.Get(); }
		TSharedRef<SWidget> TakeWidget();
		FText GetTitleText() const;

	protected:
		TWeakObjectPtr<UWorld> TargetWorld = nullptr;
		TWeakObjectPtr<ASceneCapture2D> SceneCaptureActor = nullptr;
		FSlateBrush MapBrush;
		float AccumulatedDeltaTime = 0.f;

		//------------------------
		// Property accessors
		//------------------------

		float OrthoWidth = 10000.f;
		float CaptureSize = 2048.f;
		FORCEINLINE TOptional<float> OnGetOptionalOrthoWidth() const { return OrthoWidth; }
		FORCEINLINE float GetOrthoWidth() const { return OrthoWidth; }

		void OnSetOrthoWidth(float InOrthoSize);

		FSplitterColumnSizeData MainColumns{0.75f};
		FSplitterColumnSizeData DetailsColumns{0.6f};

		FVector ReferencePosition = FVector(0, 0, 10000);
		FORCEINLINE TOptional<float> GetPositionX() const { return ReferencePosition.X; }
		FORCEINLINE TOptional<float> GetPositionY() const { return ReferencePosition.Y; }
		FORCEINLINE TOptional<float> GetPositionZ() const { return ReferencePosition.Z; }

		FORCEINLINE void OnSetPosition(float NewValue, ETextCommit::Type CommitInfo, int32 Axis)
		{
			ReferencePosition.Component(Axis) = NewValue;
		}

		FVector LocalCameraLocation = FVector::ZeroVector;
		FORCEINLINE FVector GetReferencePosition() const { return ReferencePosition + LocalCameraLocation; }

		bool bShouldFollowCamera = false;
		FORCEINLINE ECheckBoxState GetFollowCameraCheckBoxState() const
		{
			return bShouldFollowCamera ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
		}
		FORCEINLINE void OnFollowCameraCheckBoxStateChanged(ECheckBoxState CheckBoxState)
		{
			bShouldFollowCamera = CheckBoxState == ECheckBoxState::Checked;
		}

		float TickRate = 0.1f;
		FORCEINLINE TOptional<float> OnGetOptionalTickRate() const { return TickRate; }
		FORCEINLINE float GetTickRate() const { return TickRate; }
		FORCEINLINE void OnSetTickRate(float InTickRate) { TickRate = InTickRate; }

		TArray<TSharedPtr<FActorQuery>> ActorQueries;

		void AddActorQuery();
		void RemoveLastActorQuery();

		//------------------------
		// Cached Widgets
		//------------------------
		TSharedPtr<SListView<TSharedPtr<FActorQuery>>> ActorQueryListWidget;

		//------------------------
		// Widget builder functions
		//------------------------
		TSharedRef<SWidget> DetailsWidget();

		TSharedRef<ITableRow> OnGenerateActorQueryRow(
			TSharedPtr<FActorQuery> InItem,
			const TSharedRef<STableViewBase>& OwnerTable);

		TSharedRef<SWidget> MapWidget();
	};

	//------------------------------------------------------------------------
	// SActorLocationOverlay
	//------------------------------------------------------------------------

	/**
	 * The actual overlay widget that paints actor locations, names, etc.
	 * on-top of the scene capture in the background.
	 */
	class SActorLocationOverlay : public SLeafWidget
	{
		using Super = SLeafWidget;

		SLATE_BEGIN_ARGS(SActorLocationOverlay)
			{
			}

			SLATE_ATTRIBUTE(const TArray<TSharedPtr<FActorQuery>>*, ActorQueries);
			SLATE_ATTRIBUTE(FVector, ReferencePosition);
			SLATE_ATTRIBUTE(float, MapSize);
		SLATE_END_ARGS()

		TAttribute<const TArray<TSharedPtr<FActorQuery>>*> ActorQueries;
		TAttribute<FVector> ReferencePosition = FVector::ZeroVector;
		TAttribute<float> MapSize = 0.f;

		void Construct(const FArguments& InArgs);

		virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override
		{
			// No desired size. Always use maximum available space
			return FVector2D::ZeroVector;
		}

		virtual int32 OnPaint(
			const FPaintArgs& Args,
			const FGeometry& AllottedGeometry,
			const FSlateRect& MyCullingRect,
			FSlateWindowElementList& OutDrawElements,
			int32 LayerId,
			const FWidgetStyle& InWidgetStyle,
			bool bParentEnabled) const override;
	};

	//------------------------------------------------------------------------
	// SActorQueryRow
	//------------------------------------------------------------------------

	/** Slate widget for entries of a list of actor queries. */
	class SActorQueryRow : public STableRow<TSharedPtr<FActorQuery>>
	{
	public:
		SLATE_BEGIN_ARGS(SActorQueryRow)
			{
			}

			SLATE_ARGUMENT(FSplitterColumnSizeData*, ColumnSizeData)
		SLATE_END_ARGS()

		void Construct(
			const FArguments& InArgs,
			TSharedRef<STableViewBase> InOwnerTableView,
			TSharedPtr<FActorQuery>& InActorQuery);

	private:
		TSharedPtr<FActorQuery> ActorQuery;
		FSplitterColumnSizeData* ColumnSizeData = nullptr;
		FString GameplayTagQueryString;

#define DEFINE_ACTOR_MAP_TEXT_ACCESSOR(Property)                                                                       \
	FORCEINLINE FText Get##Property##_Text() const                                                                     \
	{                                                                                                                  \
		return ActorQuery.IsValid() ? FText::FromString(ActorQuery->Property) : Private::GInvalidText;                 \
	}                                                                                                                  \
	FORCEINLINE void Set##Property##_Text(const FText& Text, ETextCommit::Type) const                                  \
	{                                                                                                                  \
		if (ActorQuery.IsValid())                                                                                      \
		{                                                                                                              \
			ActorQuery->Property = Text.ToString();                                                                    \
		}                                                                                                              \
	}
		DEFINE_ACTOR_MAP_TEXT_ACCESSOR(NameFilter);
		DEFINE_ACTOR_MAP_TEXT_ACCESSOR(NameRegexPattern);
		DEFINE_ACTOR_MAP_TEXT_ACCESSOR(ActorClassName);
#undef DEFINE_ACTOR_MAP_TEXT_ACCESSOR

		FORCEINLINE FText GetGameplayTagQueryString_Text() const
		{
			return ActorQuery.IsValid() ? FText::FromString(GameplayTagQueryString) : Private::GInvalidText;
		}
		void SetGameplayTagQueryString_Text(const FText& Text, ETextCommit::Type);
	};
} // namespace OUU::Developer::ActorMapWindow::Private
