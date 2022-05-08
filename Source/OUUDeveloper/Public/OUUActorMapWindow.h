// Copyright (c) 2022 Jonas Reich

#pragma once

#include "CoreMinimal.h"

#include "Slate/SplitterColumnSizeData.h"
#include "Tickable.h"
#include "Widgets/SWidget.h"

class ASceneCapture2D;
class FActorQuery;

/**
 * The data and core functionality of the actor map window:
 * FActorMap takes care of creating objects, widgets and performing actor queries in tick.
 */
class FActorMap : public TSharedFromThis<FActorMap>, public FTickableGameObject
{
public:
	virtual ~FActorMap() override;

	// - FTickableGameObject
	virtual bool IsTickableInEditor() const override;
	virtual bool IsTickableWhenPaused() const override;
	virtual TStatId GetStatId() const override;
	virtual void Tick(float DeltaTime) override;
	// --

	/** Separate initializer outside of constructor, so shared pointer from this works as expected */
	void Initialize(UWorld* InTargetWorld);

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
