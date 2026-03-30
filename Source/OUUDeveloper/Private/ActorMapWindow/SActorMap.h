// Copyright (c) 2026 Jonas Reich & Contributors

#pragma once

#include "ActorMapWindow/OUUActorMapWindow_TabSpawner.h"
#include "Slate/SplitterColumnSizeData.h"
#include "Templates/BitmaskUtils.h"
#include "Widgets/SWidget.h"
#include "Widgets/Views/SListView.h"

class ASceneCapture2D;

namespace OUU::Developer::ActorMapWindow
{
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

		void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
		// --

		UWorld* GetDynamicTargetWorld() const;
		/** Separate initializer outside of construct so the widget can be reused for a different world */
		void InitializeForWorld(UWorld* InTargetWorld);
		FORCEINLINE UWorld* GetTargetWorld() const { return TargetWorld.Get(); }
		TSharedRef<SWidget> ConstructWidget();
		FText GetTitleText() const;

		void UpdateQueryResults();
		void UpdateSceneCapture();

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

#define DEFINE_CHECKBOX_BOOL(BoolName, DefaultValue)                                                                   \
	bool b##BoolName = DefaultValue;                                                                                   \
	FORCEINLINE ECheckBoxState Get##BoolName##CheckBoxState() const                                                    \
	{                                                                                                                  \
		return b##BoolName ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;                                      \
	}                                                                                                                  \
	FORCEINLINE void On##BoolName##CheckBoxStateChanged(ECheckBoxState CheckBoxState)                                  \
	{                                                                                                                  \
		b##BoolName = CheckBoxState == ECheckBoxState::Checked;                                                        \
	}

#if WITH_EDITOR
		DEFINE_CHECKBOX_BOOL(QueryWorldPartition, true)
#endif
		DEFINE_CHECKBOX_BOOL(FollowCamera, false)
#undef DEFINE_CHECKBOX_BOOL

#define DEFINE_CHECKBOX_SHOWFLAG(FlagName)                                                                             \
	FORCEINLINE ECheckBoxState GetDraw##FlagName##CheckBoxState() const                                                \
	{                                                                                                                  \
		return ShowFlags & EShowFlags::FlagName ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;                 \
	}                                                                                                                  \
	FORCEINLINE void OnDraw##FlagName##CheckBoxStateChanged(ECheckBoxState CheckBoxState)                              \
	{                                                                                                                  \
		if (CheckBoxState == ECheckBoxState::Checked)                                                                  \
		{                                                                                                              \
			ShowFlags |= EShowFlags::FlagName;                                                                         \
		}                                                                                                              \
		else                                                                                                           \
		{                                                                                                              \
			ShowFlags &= ~EShowFlags::FlagName;                                                                        \
		}                                                                                                              \
	}
		DEFINE_CHECKBOX_SHOWFLAG(Labels)
		DEFINE_CHECKBOX_SHOWFLAG(SceneCapture)
		DEFINE_CHECKBOX_SHOWFLAG(Distance)
#undef DEFINE_CHECKBOX_SHOWFLAG

		float TickRate = 0.1f;
		FORCEINLINE TOptional<float> OnGetOptionalTickRate() const { return TickRate; }
		FORCEINLINE float GetTickRate() const { return TickRate; }
		FORCEINLINE void OnSetTickRate(float InTickRate) { TickRate = InTickRate; }

		EShowFlags ShowFlags = EShowFlags::Default;
		EShowFlags GetShowFlags() const { return ShowFlags; }

		TArray<TSharedPtr<FOUUActorMapQuery>> ActorQueries;
		TArray<TArray<TTuple<FVector, FString>>> QueryResults;
		TArray<uint32> WorldPartitionCellIndexPerQuery;

		void AddActorQuery();
		void ClearQueries();
		void ResetQueries();
		void RebuildQueryList();
		void WriteQueriesToDefaultConfig();

		//------------------------
		// Cached Widgets
		//------------------------
		TSharedPtr<SListView<TSharedPtr<FOUUActorMapQuery>>> ActorQueryListWidget;

		//------------------------
		// Widget builder functions
		//------------------------
		TSharedRef<SWidget> DetailsWidget();

		TSharedRef<ITableRow> OnGenerateActorQueryRow(
			TSharedPtr<FOUUActorMapQuery> InItem,
			const TSharedRef<STableViewBase>& OwnerTable);

		TSharedRef<SWidget> MapWidget();
	};

} // namespace OUU::Developer::ActorMapWindow
