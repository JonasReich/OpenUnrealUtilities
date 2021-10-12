// Copyright (c) 2021 Jonas Reich

#include "CoreMinimal.h"


#include "EngineUtils.h"
#include "LogOpenUnrealUtilities.h"
#include "Brushes/SlateColorBrush.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Widgets/SWindow.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/SVectorInputBox.h"
#include "Widgets/Layout/SScaleBox.h"

FSlateColorBrush DarkGrey(FColor(6, 6, 6, 255));
FSlateColorBrush MediumGrey(FColor(13, 13, 13, 255));
FSlateColorBrush White(FColor::White);

class SActorLocationOverlay : public SLeafWidget
{
	using Super = SLeafWidget;

SLATE_BEGIN_ARGS(SActorLocationOverlay)
		{
		}

		SLATE_ATTRIBUTE(const TArray<AActor*>*, ActorList);
		SLATE_ATTRIBUTE(FVector, ReferencePosition);
		SLATE_ATTRIBUTE(float, MapSize);
		SLATE_ATTRIBUTE(FLinearColor, MarkerColor);
	SLATE_END_ARGS()

	TAttribute<const TArray<AActor*>*> ActorList;
	TAttribute<FVector> ReferencePosition = FVector::ZeroVector;
	TAttribute<float> MapSize = 0.f;
	TAttribute<FLinearColor> MarkerColor = FLinearColor::Red;

	void Construct(const FArguments& InArgs)
	{
		ActorList = InArgs._ActorList;
		ReferencePosition = InArgs._ReferencePosition;
		MapSize = InArgs._MapSize;
		MarkerColor = InArgs._MarkerColor;
	}

	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override
	{
		// No desired size. Always use maximum available space
		return FVector2D::ZeroVector;
	}

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{
		if (!ActorList.IsBound() || ActorList.Get() == nullptr)
			return LayerId;

		// Used to track the layer ID we will return.
		int32 RetLayerId = LayerId;

		const bool bEnabled = ShouldBeEnabled(bParentEnabled);
		const ESlateDrawEffect DrawEffects = bEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;

		const FLinearColor ColorAndOpacitySRGB = InWidgetStyle.GetColorAndOpacityTint();

		const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
		const FVector2D LocalCenter = LocalSize / 2;
		const float MaxComponent = LocalCenter.GetMin();
		const FVector2D CenterOffset = FVector2D(-MaxComponent, -MaxComponent);
		const FVector2D Position = LocalCenter + CenterOffset;
		const FVector2D Size = FVector2D(MaxComponent * 2.f, MaxComponent * 2.f);

		const float MapSizeActual = MapSize.Get();
		const FVector HalfMapSizeVector = FVector(MapSizeActual / 2.f, MapSizeActual / 2.f, 0);
		const FVector TopLeftCorner = ReferencePosition.Get() - HalfMapSizeVector;
		FBox BBox(TopLeftCorner, ReferencePosition.Get() + HalfMapSizeVector);
		const TArray<AActor*>& ActualActorList = *ActorList.Get();
		for (AActor* Actor : ActualActorList)
		{
			if (!IsValid(Actor))
				continue;

			const FVector WorldLocation = Actor->GetActorLocation();
			if (!BBox.IsInsideXY(WorldLocation))
				continue;

			const FVector RelativeLocation3D = WorldLocation - TopLeftCorner;
			const FVector2D RelativeRotation2D{RelativeLocation3D.X, RelativeLocation3D.Y};
			const FVector2D WidgetSpaceLocationNormalized = RelativeRotation2D / MapSizeActual;
			const FVector2D WidgetSpaceLocation = Position + WidgetSpaceLocationNormalized * Size;

			const float MarkerSize = 6.f;
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				RetLayerId++,
				AllottedGeometry.ToPaintGeometry(WidgetSpaceLocation - (MarkerSize / 2.f), FVector2D(MarkerSize, MarkerSize)),
				&White,
				DrawEffects,
				MarkerColor.Get()
			);
		}


		// Complete rect
		/*FSlateDrawElement::MakeBox(
			OutDrawElements,
			RetLayerId++,
			AllottedGeometry.ToPaintGeometry(Position, Size),
			&White,
			DrawEffects,
			ColorAndOpacitySRGB * FLinearColor(1, 1, 1, 0.1f)
		);
		*/

		/*
			// Draw all bars
			for( int32 EventIndex = 0; EventIndex < Events.Num(); ++EventIndex )
			{
				TSharedPtr< FVisualizerEvent > Event = Events[ EventIndex ];
				float StartX, EndX;
				if( CalculateEventGeometry( Event.Get(), AllottedGeometry, StartX, EndX ) )
				{
					// Draw Event bar
					FSlateDrawElement::MakeBox(
						OutDrawElements,
						RetLayerId++,
						AllottedGeometry.ToPaintGeometry(
							FVector2D( StartX, 0.0f ),
							FVector2D( EndX - StartX, AllottedGeometry.GetLocalSize().Y )),
						Event->IsSelected ? SelectedImage : FillImage,
						DrawEffects,
						Event->IsSelected ? SelectedBarColor : ColorPalette[Event->ColorIndex % (sizeof(ColorPalette) / sizeof(ColorPalette[0]))]
					);
				}
			}
	*/
		return RetLayerId - 1;
	}
};

class FActorMap : public TSharedFromThis<FActorMap>
{
public:
	~FActorMap()
	{
		if (SceneCaptureActor.IsValid())
		{
			SceneCaptureActor->Destroy();
			SceneCaptureActor.Reset();
		}

		MapBrush.SetResourceObject(nullptr);
	}

	/** Separate initializer outside of constructor, so shared pointer from this works as expected */
	void Initialize(UWorld* InTargetWorld)
	{
		check(IsValid(InTargetWorld));

		TargetWorld = InTargetWorld;

		// Look down
		const FRotator Direction(-90, -90, 0);
		SceneCaptureActor = TargetWorld->SpawnActor<ASceneCapture2D>(ReferencePosition, Direction);
		auto* CaptureComponent = SceneCaptureActor->GetCaptureComponent2D();

		// #TODO-j.reich Expose update rate via controlled ticking, etc
		CaptureComponent->bCaptureEveryFrame = true;
		CaptureComponent->ProjectionType = ECameraProjectionMode::Orthographic;
		CaptureComponent->OrthoWidth = OrthoWidth;
		CaptureComponent->bEnableClipPlane = false;

		const FName TargetName = MakeUniqueObjectName(SceneCaptureActor.Get(), UTextureRenderTarget2D::StaticClass(), TEXT("SceneCaptureTextureTarget"));
		CaptureComponent->TextureTarget = NewObject<UTextureRenderTarget2D>(SceneCaptureActor.Get(), TargetName);
		CaptureComponent->TextureTarget->InitCustomFormat(CaptureSize, CaptureSize, PF_FloatRGB, false);
		CaptureComponent->TextureTarget->ClearColor = FLinearColor::Black;
		CaptureComponent->TextureTarget->TargetGamma = 2.2f;

		CaptureComponent->CaptureScene();

		MapBrush = FSlateBrush();
		MapBrush.SetResourceObject(CaptureComponent->TextureTarget);
		MapBrush.ImageSize.X = CaptureComponent->TextureTarget->Resource->GetSizeX();
		MapBrush.ImageSize.Y = CaptureComponent->TextureTarget->Resource->GetSizeY();

		MainColumns.LeftColumnWidth = TAttribute<float>(this, &FActorMap::OnGetDetailsWidth);
		MainColumns.RightColumnWidth = TAttribute<float>(this, &FActorMap::OnGetMapWidth);
		MainColumns.OnWidthChanged = SSplitter::FOnSlotResized::CreateSP(this, &FActorMap::OnSetMapWidth);

		DetailsColumns.LeftColumnWidth = TAttribute<float>(this, &FActorMap::OnGetLeftDetailsColumnWidth);
		DetailsColumns.RightColumnWidth = TAttribute<float>(this, &FActorMap::OnGetRightDetailsColumnWidth);
		DetailsColumns.OnWidthChanged = SSplitter::FOnSlotResized::CreateSP(this, &FActorMap::OnSetDetailsColumnWidth);

		// #TODO-j.reich Update the actor list some place else!
		UpdateActorList();
	}

	UWorld* GetTargetWorld() const
	{
		return TargetWorld.Get();
	}

	TSharedRef<SWidget> TakeWidget()
	{
		return SNew(SBorder)
		.BorderImage(&DarkGrey)
		.Content()
		[
			SNew(SSplitter)
			+ SSplitter::Slot()
			  .SizeRule(SSplitter::ESizeRule::FractionOfParent)
			  .Value(MainColumns.LeftColumnWidth)
			  .OnSlotResized(MainColumns.OnWidthChanged)
			[
				SNew(SBorder)
				.Padding(2.f)
				.BorderImage(&MediumGrey)
				.Content()
				[
					SNew(SBox)
					.Padding(2.f)
					.Content()
					[
						DetailsWidget()
					]
				]
			]
			+ SSplitter::Slot()
			  .SizeRule(SSplitter::ESizeRule::FractionOfParent)
			  .Value(MainColumns.RightColumnWidth)
			  .OnSlotResized(MainColumns.OnWidthChanged)
			[
				SNew(SBorder)
				.Padding(2.f)
				.BorderImage(&MediumGrey)
				.Content()
				[
					SNew(SBox)
					.Padding(2.f)
					.Content()
					[
						MapWidget()
					]
				]
			]
		];
	}

	FText GetTitleText() const
	{
		return FText::FromString(FString::Printf(TEXT("OUU Actor Map (%s) [%s]"),
		                                         *TargetWorld->GetName(), GetWorldTypeString(TargetWorld->WorldType)));
	}

protected:
	TWeakObjectPtr<UWorld> TargetWorld = nullptr;
	TWeakObjectPtr<ASceneCapture2D> SceneCaptureActor = nullptr;
	FSlateBrush MapBrush;

	static const TCHAR* GetWorldTypeString(EWorldType::Type Type)
	{
		switch (Type)
		{
		case EWorldType::None: return TEXT("None");
		case EWorldType::Game: return TEXT("Game");
		case EWorldType::Editor: return TEXT("Editor");
		case EWorldType::PIE: return TEXT("PIE");
		case EWorldType::EditorPreview: return TEXT("EditorPreview");
		case EWorldType::GamePreview: return TEXT("GamePreview");
		case EWorldType::GameRPC: return TEXT("GameRPC");
		case EWorldType::Inactive: return TEXT("Inactive");
		default: return TEXT("Unknown");
		}
	}

	//------------------------
	// Property accessors
	//------------------------

	float OrthoWidth = 10000.f;
	float CaptureSize = 2048.f;
	TOptional<float> OnGetOptionalOrthoWidth() const { return OrthoWidth; }
	float GetOrthoWidth() const	{ return OrthoWidth; }

	void OnSetOrthoWidth(float InOrthoSize)
	{

		OrthoWidth = InOrthoSize;
		// #TODO-j.reich Move into proper tick
		if (SceneCaptureActor.IsValid())
		{
			SceneCaptureActor->GetCaptureComponent2D()->OrthoWidth = OrthoWidth;
		}
	}


	struct FColumnSizeData
	{
		TAttribute<float> LeftColumnWidth;
		TAttribute<float> RightColumnWidth;
		SSplitter::FOnSlotResized OnWidthChanged;

		void SetColumnWidth(float InWidth) { OnWidthChanged.ExecuteIfBound(InWidth); }
	};

	FColumnSizeData MainColumns;
	float MapColumnWidthFactor = 0.9f;
	float OnGetDetailsWidth() const { return 1.0f - MapColumnWidthFactor; }
	float OnGetMapWidth() const { return MapColumnWidthFactor; }
	void OnSetMapWidth(float InWidth) { MapColumnWidthFactor = InWidth; }

	FColumnSizeData DetailsColumns;
	float DetailsColumnWidthFactor = 0.6f;
	float OnGetLeftDetailsColumnWidth() const { return 1.0f - DetailsColumnWidthFactor; }
	float OnGetRightDetailsColumnWidth() const { return DetailsColumnWidthFactor; }
	void OnSetDetailsColumnWidth(float InWidth) { DetailsColumnWidthFactor = InWidth; }

	FVector ReferencePosition = FVector(0, 0, 10000);
	TOptional<float> GetPositionX() const { return ReferencePosition.X; }
	TOptional<float> GetPositionY() const { return ReferencePosition.Y; }
	TOptional<float> GetPositionZ() const { return ReferencePosition.Z; }
	FVector GetReferencePosition() const { return ReferencePosition; } 

	void OnSetPosition(float NewValue, ETextCommit::Type CommitInfo, int32 Axis)
	{
		ReferencePosition.Component(Axis) = NewValue;

		// #TODO-j.reich Move into proper tick
		if (SceneCaptureActor.IsValid())
		{
			SceneCaptureActor->SetActorLocation(ReferencePosition);
		}
	}

	TArray<AActor*> ActorList;
	const TArray<AActor*>* GetActorList() const
	{
		return &ActorList;
	}

	// #TODO-j.reich This must be done regularly in tick
	// #TODO-j.reich We need to be able to support multiple actor lists. Need to check on which level!
	void UpdateActorList()
	{
		ActorList.Empty();
		for (AActor* Actor : TActorRange<AActor>(GetTargetWorld()))
		{
			// #TODO-j.reich Implement actor filters

			// Ignore AInfo actors, because they never have a valid location
			if (Actor->IsA<AInfo>())
				continue;

			if (!Actor->IsA<AStaticMeshActor>())
				continue;

			ActorList.Add(Actor);
		}
	}

	//------------------------
	// Widget builder functions
	//------------------------
	TSharedRef<SWidget> DetailsWidget()
	{
		return SNew(SBox)
        .MinDesiredWidth(200.f)
        .Content()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				DetailsSplitter(
					INVTEXT("Ortho Width"),
					INVTEXT("Orthographic height and width of the actor map. Always assumes a square map / render target background"),
					SNew(SNumericEntryBox<float>)
					.Value(this, &FActorMap::OnGetOptionalOrthoWidth)
					.OnValueChanged(this, &FActorMap::OnSetOrthoWidth)
				)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				DetailsSplitter(
					INVTEXT("Origin"),
					INVTEXT("The position from which the render capture of the world is made"),
					SNew(SVectorInputBox)
        			.X(this, &FActorMap::GetPositionX)
        			.Y(this, &FActorMap::GetPositionY)
        			.Z(this, &FActorMap::GetPositionZ)
        			.AllowResponsiveLayout(true)
        			.AllowSpin(true)
        			.OnXCommitted(this, &FActorMap::OnSetPosition, 0)
        			.OnYCommitted(this, &FActorMap::OnSetPosition, 1)
        			.OnZCommitted(this, &FActorMap::OnSetPosition, 2)
				)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				DetailsSplitter(
					INVTEXT("UPDATE RATE"),
					INVTEXT("..."),
					SNullWidget::NullWidget
				)
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.f)
			[
				DetailsSplitter(
					INVTEXT("VISUALIZER LIST"),
					INVTEXT("..."),
					SNullWidget::NullWidget
				)
			]
		];
	}

	TSharedRef<SWidget> DetailsSplitter(FText Label, FText Tooltip, TSharedRef<SWidget> RightWidget)
	{
		return SNew(SSplitter)
			+ SSplitter::Slot()
			  .SizeRule(SSplitter::ESizeRule::FractionOfParent)
			  .Value(DetailsColumns.LeftColumnWidth)
			  .OnSlotResized(DetailsColumns.OnWidthChanged)
			[
				SNew(STextBlock)
				.Text(Label)
				.ToolTipText(Tooltip)
			]
			+ SSplitter::Slot()
			  .SizeRule(SSplitter::ESizeRule::FractionOfParent)
			  .Value(DetailsColumns.RightColumnWidth)
			  .OnSlotResized(DetailsColumns.OnWidthChanged)
			[
				RightWidget
			];
	}

	TSharedRef<SWidget> MapWidget()
	{
		return
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SScaleBox)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				.Stretch(EStretch::ScaleToFit)
				.Content()
				[
					SNew(SImage)
					.Image(&MapBrush)
				]
			]
			+ SOverlay::Slot()
			[
				SNew(SActorLocationOverlay)
				.ActorList(this, &FActorMap::GetActorList)
				.MarkerColor(FLinearColor::Green)
				.MapSize(this, &FActorMap::GetOrthoWidth)
				.ReferencePosition(this, &FActorMap::GetReferencePosition)
			];
	}

	void HandleButtonPressed()
	{
		UE_LOG(LogTemp, Log, TEXT("world type: %s"), GetWorldTypeString(TargetWorld->WorldType));
	}
};


class FActorMapWindowBootstrapper
{
public:
	TSharedPtr<FActorMap> ActorMap;

	DECLARE_EVENT(FActorMapWindowBootstrapper, FOnSlateWindowClosed);

	FOnSlateWindowClosed OnWindowClosed;

	TSharedPtr<SWindow> SlateWindow;

	static UWorld* GetCurrentTargetWorld()
	{
		// Always prefer the play world (both in cooked game and in PIE)
		if (UWorld* PossibleResult = GEngine->GetCurrentPlayWorld())
			return PossibleResult;

#if WITH_EDITOR
		if (GIsEditor)
		{
			// Fallback to the editor world in the editor
			return GEditor->GetEditorWorldContext().World();
		}
#endif
		return nullptr;
	}

	void HandleSlateWindowClosed(const TSharedRef<SWindow>& ClosedWindow)
	{
		ensure(SlateWindow.ToSharedRef() == ClosedWindow);
		HandleSlateWindowClosed_Inner();
	}

	void HandleSlateWindowClosed_Inner()
	{
		ActorMap.Reset();
		if (SlateWindow.IsValid())
		{
			SlateWindow.Reset();
			OnWindowClosed.Broadcast();
		}
	}

	void OpenWindowForCurrentWorld()
	{
		checkf(!ActorMap.IsValid(), TEXT("OpenWindowForCurrentWorld() must not be called twice on the same object"));

		ActorMap = MakeShared<FActorMap>();
		ActorMap->Initialize(GetCurrentTargetWorld());
		GEngine->OnWorldDestroyed().AddRaw(this, &FActorMapWindowBootstrapper::HandleWorldDestroyed);

		SlateWindow = SNew(SWindow)
			.AutoCenter(EAutoCenter::None)
			.IsInitiallyMaximized(true)
			.ScreenPosition({20.f, 20.f})
			.CreateTitleBar(true)
			.SizingRule(ESizingRule::UserSized)
			.SupportsMaximize(true)
			.SupportsMinimize(true)
			.HasCloseButton(true)
			.Style(&FCoreStyle::Get().GetWidgetStyle<FWindowStyle>("Window"))
			.ClientSize({500.f, 300.f})
			.UseOSWindowBorder(false)
			.Title(ActorMap->GetTitleText());

		SlateWindow->SetContent(ActorMap->TakeWidget());

		SlateWindow->SetOnWindowClosed(FOnWindowClosed::CreateRaw(this, &FActorMapWindowBootstrapper::HandleSlateWindowClosed));

		FSlateApplication::Get().AddWindow(SlateWindow.ToSharedRef());
		FSlateApplication::Get().GetRenderer()->CreateViewport(SlateWindow.ToSharedRef());
	}

	void HandleWorldDestroyed(UWorld* WorldDestroyed)
	{
		if (!ensure(IsValid(WorldDestroyed)))
			return;

		// Automatically close the window once the world is destroyed.
		if (WorldDestroyed == ActorMap->GetTargetWorld())
			CloseWindow();
	}

	void CloseWindow()
	{
		if (SlateWindow.IsValid())
		{
			SlateWindow->DestroyWindowImmediately();
			HandleSlateWindowClosed_Inner();
		}
	}
};

// Use unique pointer for now, so we only have to support a single window using the cheat.
TUniquePtr<FActorMapWindowBootstrapper> ActorMapWindowBootstrapper;

void OpenActorMapWindowForCurrentWorld()
{
	if (ActorMapWindowBootstrapper.IsValid())
	{
		UE_LOG(LogOpenUnrealUtilities, Warning, TEXT("OpenActorMapWindowForCurrentWorld() was called, but a different window is already opened. Closing the previous instance..."))
		ActorMapWindowBootstrapper->CloseWindow();
	}
	ActorMapWindowBootstrapper = MakeUnique<FActorMapWindowBootstrapper>();
	// Automatically clean up unique pointer once the window is closed.
	ActorMapWindowBootstrapper->OnWindowClosed.AddLambda([]()
	{
		ActorMapWindowBootstrapper.Reset();
	});
	ActorMapWindowBootstrapper->OpenWindowForCurrentWorld();
}

static FAutoConsoleCommand OpenActorMapCommand(
	TEXT("ouu.Debug.OpenActorMap"),
	TEXT("Open an actor map for the current world (game or editor)"),
	FConsoleCommandDelegate::CreateStatic(OpenActorMapWindowForCurrentWorld)
);
