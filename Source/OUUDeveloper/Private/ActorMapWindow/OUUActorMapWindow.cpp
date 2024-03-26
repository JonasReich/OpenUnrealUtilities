// Copyright (c) 2023 Jonas Reich & Contributors

#include "ActorMapWindow/OUUActorMapWindow.h"

#include "CoreMinimal.h"

#include "AbilitySystemComponent.h"
#include "ActorMapWindow/OUUActorMapWindow_Private.h"
#include "Brushes/SlateColorBrush.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/Engine.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "EngineUtils.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Docking/TabManager.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "GameplayTags/GameplayTagQueryParser.h"
#include "Misc/EngineVersionComparison.h"
#include "Misc/RegexUtils.h"
#include "TextureResource.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/SVectorInputBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SWindow.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"

#if WITH_EDITOR
	#include "LevelEditorViewport.h"
	#include "WorkspaceMenuStructure.h"
	#include "WorkspaceMenuStructureModule.h"
#endif

UE_DISABLE_OPTIMIZATION

namespace OUU::Developer::ActorMapWindow
{
	namespace Private
	{
		/**
		 * Hard coded editor colors, do not update with editor style config,
		 * but I did not want to deal with that at this time..
		 */
		FSlateColorBrush DarkGrey(FColor(6, 6, 6, 255));
		FSlateColorBrush MediumGrey(FColor(13, 13, 13, 255));
		FSlateColorBrush White(FColor::White);
		FColor LabelBackgroundColor(0, 0, 0, 200);

		/**
		 * Default colors for actor overlays.
		 * The list contains the most extreme saturated colors only to make the stand out as much as possible.
		 */
		TArray<FColor> DefaultColors = {
			FColorList::Red,
			FColorList::Green,
			FColorList::Blue,
			FColorList::Magenta,
			FColorList::Cyan,
			FColorList::Yellow};

		FText GInvalidText = INVTEXT("<invalid>");

		UWorld* GetDynamicTargetWorld()
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

		TSharedRef<SDockTab> SpawnTab(const FSpawnTabArgs& Args)
		{
			// clang-format off
			return SNew(SDockTab)
			.Label(ActorMapWindow::GTabTitle)
			[
				SNew(SActorMap)
			];
			// clang-format on
		}

		//------------------------------------------------------------------------
		// SActorLocationOverlay
		//------------------------------------------------------------------------

		void SActorLocationOverlay::Construct(const FArguments& InArgs)
		{
			ActorQueries = InArgs._ActorQueries;
			ReferencePosition = InArgs._ReferencePosition;
			MapSize = InArgs._MapSize;
			DrawLabels = InArgs._DrawLabels;
		}

		int32 SActorLocationOverlay::OnPaint(
			const FPaintArgs& Args,
			const FGeometry& AllottedGeometry,
			const FSlateRect& MyCullingRect,
			FSlateWindowElementList& OutDrawElements,
			int32 LayerId,
			const FWidgetStyle& InWidgetStyle,
			bool bParentEnabled) const
		{
			if (ActorQueries.Get() == nullptr)
				return LayerId;

			// Used to track the layer ID we will return.
			int32 RetLayerId = LayerId;

			const bool bEnabled = ShouldBeEnabled(bParentEnabled);
			const ESlateDrawEffect DrawEffects = bEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;

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
			auto ActualActorQueries = *ActorQueries.Get();

			const TSharedRef<FSlateFontMeasure> FontMeasure =
				FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
			auto& Style = FCoreStyle::Get().GetWidgetStyle<FTextBlockStyle>("SmallText");
			const FSlateFontInfo FontInfo = Style.Font;

			for (auto Query : ActualActorQueries)
			{
				if (!Query.IsValid())
					continue;

				for (AActor* Actor : Query->CachedQueryResult.Actors)
				{
					if (!IsValid(Actor))
						continue;

					const FVector WorldLocation = Actor->GetActorLocation();
					if (!BBox.IsInsideXY(WorldLocation))
						continue;

					const FVector RelativeLocation3D = WorldLocation - TopLeftCorner;
					const FVector2D RelativeLocation2D{RelativeLocation3D.X, RelativeLocation3D.Y};
					const FVector2D RelativeLocation2D_Normalized = RelativeLocation2D / MapSizeActual;
					// Need to remap coordinates from world space when looking down (x is up, y is right) to UI space (x
					// is right, y is down)
					const FVector2D WidgetSpaceLocationNormalized{
						RelativeLocation2D_Normalized.Y,
						1.f - RelativeLocation2D_Normalized.X};
					const FVector2D WidgetSpaceLocation = Position + WidgetSpaceLocationNormalized * Size;

					const float MarkerSize = 6.f;
					FSlateDrawElement::MakeBox(
						OutDrawElements,
						RetLayerId++,
						AllottedGeometry.ToPaintGeometry(
							FVector2D(MarkerSize, MarkerSize),
							FSlateLayoutTransform(1.f, WidgetSpaceLocation - (MarkerSize / 2.f))),
						&Private::White,
						DrawEffects,
						Query->QueryColor);

					if (this->DrawLabels.Get() == ECheckBoxState::Checked)
					{
						FText LabelText = FText::FromString(Actor->GetActorNameOrLabel());

						FVector2f LabelDimensions = FontMeasure->Measure(LabelText, FontInfo);

						FSlateDrawElement::MakeBox(
							OutDrawElements,
							RetLayerId++,
							AllottedGeometry.ToPaintGeometry(
								FVector2D(LabelDimensions),
								FSlateLayoutTransform(
									1.f,
									WidgetSpaceLocation - (MarkerSize / 2.f) + FVector2D(0, MarkerSize))),
							&Private::White,
							DrawEffects,
							Private::LabelBackgroundColor);

						FSlateDrawElement::MakeText(
							OutDrawElements,
							RetLayerId++,
							AllottedGeometry.ToPaintGeometry(
								FVector2D(FVector2f(MarkerSize, MarkerSize)),
								FSlateLayoutTransform(
									1.f,
									WidgetSpaceLocation - (MarkerSize / 2.f) + FVector2D(0, MarkerSize))),
							LabelText,
							FontInfo,
							DrawEffects,
							Query->QueryColor);
					}
				}
			}

			return RetLayerId - 1;
		}

		//------------------------------------------------------------------------
		// SActorQueryRow
		//------------------------------------------------------------------------

		void SActorQueryRow::Construct(
			const FArguments& InArgs,
			TSharedRef<STableViewBase> InOwnerTableView,
			TSharedPtr<FActorQuery>& InActorQuery)
		{
			ActorQuery = InActorQuery;
			ensure(ActorQuery.IsValid());

			ColumnSizeData = InArgs._ColumnSizeData;

			// clang-format off
		STableRow<TSharedPtr<FActorQuery>>::Construct(
			STableRow<TSharedPtr<FActorQuery>>::FArguments()
			.Padding(FMargin(4.f, 2.f))
			.Content()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.Padding(4.f, 2.f)
				[
					SNew(SColorBlock)
					.Color(ActorQuery->QueryColor)
				]
				+ SVerticalBox::Slot()
				.Padding(4.f, 2.f)
				[
					ColumnSizeData->MakeSimpleDetailsSplitter(
						INVTEXT("Name Filter"),
						INVTEXT("Name string that must be contained within the actor names, for the actor to be "
							"included in the query."),
						SNew(SEditableTextBox)
						.Text(this, &SActorQueryRow::GetNameFilter_Text)
						.HintText(INVTEXT("<empty>"))
						.OnTextCommitted(this, &SActorQueryRow::SetNameFilter_Text)
					)
				]
				+ SVerticalBox::Slot()
				.Padding(4.f, 2.f)
				[
					ColumnSizeData->MakeSimpleDetailsSplitter(
						INVTEXT("Name Regex Pattern"),
						INVTEXT("Regular expression pattern that must match to actor names, for the actor to be "
							"included in the query."),
						SNew(SEditableTextBox)
						.Text(this, &SActorQueryRow::GetNameRegexPattern_Text)
						.HintText(INVTEXT("<empty>"))
						.OnTextCommitted(this, &SActorQueryRow::SetNameRegexPattern_Text)
					)
				]
				+ SVerticalBox::Slot()
				.Padding(4.f, 2.f)
				[
					ColumnSizeData->MakeSimpleDetailsSplitter(
						INVTEXT("Class Filter"),
						INVTEXT("Name string that must match the actors class name or any of its "
							"super classes, for the actor to be included in the query."),
						SNew(SEditableTextBox)
						.Text(this, &SActorQueryRow::GetActorClassName_Text)
						.HintText(INVTEXT("<empty>"))
						.OnTextCommitted(this, &SActorQueryRow::SetActorClassName_Text)
					)
				]
				+ SVerticalBox::Slot()
				.Padding(4.f, 2.f)
				[
					ColumnSizeData->MakeSimpleDetailsSplitter(
						INVTEXT("Component Class Filter"),
						INVTEXT("Name string that must match any of the actor component class names or any of their "
							"super classes, for the actor to be included in the query."),
						SNew(SEditableTextBox)
						.Text(this, &SActorQueryRow::GetComponentClassName_Text)
						.HintText(INVTEXT("<empty>"))
						.OnTextCommitted(this, &SActorQueryRow::SetComponentClassName_Text)
					)
				]
				+ SVerticalBox::Slot()
				.Padding(4.f, 2.f)
				[
					ColumnSizeData->MakeSimpleDetailsSplitter(
						INVTEXT("Gameplay Tag Query"),
						INVTEXT("Gameplay tag query. Must use fully qualified tag names that can be combined with ANY(), ALL() and NONE() operators.\ne.g. NONE(ANY(Foo.Bar, Foo.Baz), FooBar)"),
						SNew(SEditableTextBox)
						.Text(this, &SActorQueryRow::GetGameplayTagQueryString_Text)
						.HintText(INVTEXT("<empty>"))
						.OnTextCommitted(this, &SActorQueryRow::SetGameplayTagQueryString_Text)
					)
				]
				+ SVerticalBox::Slot()
				.Padding(4.f, 2.f)
				[
					SNew(STextBlock).Text_Lambda([this]()
					{
						return ActorQuery.IsValid() ? FText::Format(INVTEXT("Num Actors: {0}"), ActorQuery->CachedQueryResult.Actors.Num()) : Private::GInvalidText;
					})
				]
			], InOwnerTableView);
			// clang-format on
		}

		void SActorQueryRow::SetGameplayTagQueryString_Text(const FText& Text, ETextCommit::Type)
		{
			FString TextAsString = Text.ToString();
			if (TextAsString != GameplayTagQueryString)
			{
				GameplayTagQueryString = TextAsString;
				if (ActorQuery.IsValid())
				{
					ActorQuery->ActorTagQuery = FGameplayTagQueryParser::ParseQuery(GameplayTagQueryString);
				}
			}
		}

		//------------------------------------------------------------------------
		// FActorMapImpl
		//------------------------------------------------------------------------

		SActorMap::~SActorMap()
		{
			if (SceneCaptureActor.IsValid())
			{
				SceneCaptureActor->Destroy();
				SceneCaptureActor.Reset();
			}

			MapBrush.SetResourceObject(nullptr);
		}

		void SActorMap::Construct(const FArguments& InArgs)
		{
			InitializeForWorld(GetDynamicTargetWorld());
			this->ChildSlot[TakeWidget()];
		}

		void SActorMap::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
		{
			Super::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

			// Magic number for delta time that we ignore.
			// I only want to update the actor map if the DT is smaller than a second.
			// Otherwise there's bigger fish to fry.
			constexpr float MagicDeltaTimeLimit = 1000;
			if (!ensure(TickRate > 0) || InDeltaTime > MagicDeltaTimeLimit)
				return;

			AccumulatedDeltaTime += InDeltaTime;

			if (AccumulatedDeltaTime >= TickRate)
			{
				while (AccumulatedDeltaTime >= TickRate)
				{
					AccumulatedDeltaTime -= TickRate;
				}

				if (SceneCaptureActor.IsValid())
				{
					LocalCameraLocation = FVector::ZeroVector;
					if (bFollowCamera)
					{
						bool bSetLocalCameraLocation = false;
						if (auto* LocalPlayerController = TargetWorld->GetFirstPlayerController())
						{
#if UE_VERSION_OLDER_THAN(5, 0, 0)
							if (auto* Camera = LocalPlayerController->PlayerCameraManager)
#else
							if (APlayerCameraManager* Camera = LocalPlayerController->PlayerCameraManager.Get())
#endif
							{
								bSetLocalCameraLocation = true;
								LocalCameraLocation = Camera->GetCameraLocation();
							}
						}
						if (!bSetLocalCameraLocation)
						{
#if WITH_EDITOR
							for (FLevelEditorViewportClient* LevelVC : GEditor->GetLevelViewportClients())
							{
								if (LevelVC && LevelVC->IsPerspective())
								{
									LocalCameraLocation = LevelVC->GetViewLocation();
								}
							}
#endif
						}
					}

					SceneCaptureActor->SetActorLocation(LocalCameraLocation + ReferencePosition);
					SceneCaptureActor->GetCaptureComponent2D()->CaptureScene();
				}

				// Update the actor queries
				UWorld* World = TargetWorld.Get();
				if (!IsValid(World))
					return;

				for (auto Query : ActorQueries)
				{
					if (!Query.IsValid())
						continue;

					Query->ExecuteAndCacheQuery(World);
				}
			}
		}

		void SActorMap::InitializeForWorld(UWorld* InTargetWorld)
		{
			check(IsValid(InTargetWorld));

			TargetWorld = InTargetWorld;

			// Look down
			const FRotator Direction(-90, 0, 0);
			SceneCaptureActor = TargetWorld->SpawnActor<ASceneCapture2D>(ReferencePosition, Direction);
			auto* CaptureComponent = SceneCaptureActor->GetCaptureComponent2D();

			CaptureComponent->bCaptureEveryFrame = false;
			CaptureComponent->bCaptureOnMovement = false;
			CaptureComponent->ProjectionType = ECameraProjectionMode::Orthographic;
			CaptureComponent->OrthoWidth = OrthoWidth;
			CaptureComponent->CaptureSource = ESceneCaptureSource::SCS_BaseColor;
			CaptureComponent->bEnableClipPlane = false;

			const FName TargetName = MakeUniqueObjectName(
				SceneCaptureActor.Get(),
				UTextureRenderTarget2D::StaticClass(),
				TEXT("SceneCaptureTextureTarget"));
			CaptureComponent->TextureTarget = NewObject<UTextureRenderTarget2D>(SceneCaptureActor.Get(), TargetName);
			CaptureComponent->TextureTarget->InitCustomFormat(CaptureSize, CaptureSize, PF_FloatRGB, false);
			CaptureComponent->TextureTarget->ClearColor = FLinearColor::Black;
			CaptureComponent->TextureTarget->TargetGamma = 2.2f;

			CaptureComponent->CaptureScene();

			MapBrush = FSlateBrush();
			MapBrush.SetResourceObject(CaptureComponent->TextureTarget);
			MapBrush.ImageSize.X = CaptureComponent->TextureTarget->GetResource()->GetSizeX();
			MapBrush.ImageSize.Y = CaptureComponent->TextureTarget->GetResource()->GetSizeY();
		}

		TSharedRef<SWidget> SActorMap::TakeWidget()
		{
			// clang-format off
	return SNew(SBorder).BorderImage(&Private::DarkGrey).Content()[SNew(SSplitter)
		+ SSplitter::Slot()
		.SizeRule(SSplitter::ESizeRule::FractionOfParent)
		.Value(MainColumns.LeftColumnWidth)
		.OnSlotResized(MainColumns.OnWidthChanged)
		[
			SNew(SBorder)
				.Padding(2.f)
				.BorderImage(&Private::MediumGrey)
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
				.BorderImage(&Private::MediumGrey)
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
			// clang-format on
		}

		FText SActorMap::GetTitleText() const
		{
			return FText::FromString(FString::Printf(
				TEXT("OUU Actor Map (%s) [%s]"),
				*TargetWorld->GetName(),
				GetWorldTypeString(TargetWorld->WorldType)));
		}

		void SActorMap::OnSetOrthoWidth(float InOrthoSize)
		{
			OrthoWidth = InOrthoSize;
			if (SceneCaptureActor.IsValid())
			{
				SceneCaptureActor->GetCaptureComponent2D()->OrthoWidth = OrthoWidth;
			}
		}

		void SActorMap::AddActorQuery()
		{
			const int32 NewIndex = ActorQueries.Add(MakeShared<FActorQuery>());
			ActorQueries[NewIndex]->QueryColor = Private::DefaultColors[NewIndex % Private::DefaultColors.Num()];
			if (ActorQueryListWidget.IsValid())
			{
				ActorQueryListWidget->RebuildList();
			}
		}

		void SActorMap::RemoveLastActorQuery()
		{
			if (ActorQueries.Num() > 0)
			{
				ActorQueries.Pop();
			}
			if (ActorQueryListWidget.IsValid())
			{
				ActorQueryListWidget->RebuildList();
			}
		}

		TSharedRef<SWidget> SActorMap::DetailsWidget()
		{
			// clang-format off
	return SNew(SBox)
		.MinDesiredWidth(200.f)
		.Content()
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			DetailsColumns.MakeSimpleDetailsSplitter(
				INVTEXT("Ortho Width"),
				INVTEXT("Orthographic height and width of the actor map. Always assumes a square map / render target background"),
				SNew(SNumericEntryBox<float>)
					.Value(this, &SActorMap::OnGetOptionalOrthoWidth)
					.OnValueChanged(this, &SActorMap::OnSetOrthoWidth)
			)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			DetailsColumns.MakeSimpleDetailsSplitter(
				INVTEXT("Origin"),
				INVTEXT("The position from which the render capture of the world is made"),
				SNew(SVectorInputBox)
					.X(this, &SActorMap::GetPositionX)
					.Y(this, &SActorMap::GetPositionY)
					.Z(this, &SActorMap::GetPositionZ)
					.AllowSpin(true)
					.OnXCommitted(this, &SActorMap::OnSetPosition, 0)
					.OnYCommitted(this, &SActorMap::OnSetPosition, 1)
					.OnZCommitted(this, &SActorMap::OnSetPosition, 2)
			)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			DetailsColumns.MakeSimpleDetailsSplitter(
				INVTEXT("Follow Camera"),
				INVTEXT("If to apply the Origin relative to the location of the currently possessed camera"),
				SNew(SCheckBox)
					.IsChecked(this, &SActorMap::GetFollowCameraCheckBoxState)
					.OnCheckStateChanged(this, &SActorMap::OnFollowCameraCheckBoxStateChanged)
			)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			DetailsColumns.MakeSimpleDetailsSplitter(
				INVTEXT("Tick Rate"),
				INVTEXT("Time between two map updates in seconds"),
				SNew(SNumericEntryBox<float>)
					.Value(this, &SActorMap::OnGetOptionalTickRate)
					.OnValueChanged(this, &SActorMap::OnSetTickRate)
			)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			DetailsColumns.MakeSimpleDetailsSplitter(
				INVTEXT("Draw Labels"),
				INVTEXT("If to draw name labels next to the markers on the map"),
				SNew(SCheckBox)
					.IsChecked(this, &SActorMap::GetDrawLabelsCheckBoxState)
					.OnCheckStateChanged(this, &SActorMap::OnDrawLabelsCheckBoxStateChanged)
			)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SSpacer)
			.Size(FVector2D{0.f, 20.f})
		]
		+ SVerticalBox::Slot()
		.Padding(0.f, 4.f)
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			[
				SNew(SButton)
					.Text(INVTEXT("Add Actor Query"))
					.OnPressed(this, &SActorMap::AddActorQuery)
			]
			+ SHorizontalBox::Slot()
			[
				SNew(SButton)
					.Text(INVTEXT("Remove Last Actor Query"))
					.OnPressed(this, &SActorMap::RemoveLastActorQuery)
			]
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		[
			SAssignNew(ActorQueryListWidget, SListView<TSharedPtr<FActorQuery>>)
				.ListItemsSource(&ActorQueries)
				.OnGenerateRow(this, &SActorMap::OnGenerateActorQueryRow)
		]
	];
			// clang-format on
		}

		TSharedRef<ITableRow> SActorMap::OnGenerateActorQueryRow(
			TSharedPtr<FActorQuery> InItem,
			const TSharedRef<STableViewBase>& OwnerTable)
		{
			return SNew(SActorQueryRow, OwnerTable, InItem).ColumnSizeData(&DetailsColumns);
		}

		TSharedRef<SWidget> SActorMap::MapWidget()
		{
			// clang-format off
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
				.ActorQueries(&ActorQueries)
				.MapSize(this, &SActorMap::GetOrthoWidth)
				.DrawLabels(this, &SActorMap::GetDrawLabelsCheckBoxState)
				.ReferencePosition(this, &SActorMap::GetReferencePosition)
		];
			// clang-format on
		}
	} // namespace Private

	//------------------------------------------------------------------------
	// Public Implementation
	//------------------------------------------------------------------------

	FName GTabName = TEXT("OUUActorMap");
	FText GTabTitle = INVTEXT("OUU Actor Map");

	void RegisterNomadTabSpawner()
	{
		FGlobalTabmanager::Get()
			->RegisterNomadTabSpawner(GTabName, FOnSpawnTab::CreateStatic(&Private::SpawnTab))
			.SetDisplayName(GTabTitle)
			.SetTooltipText(
				INVTEXT("View a top-down overview of actors in a level (editor or runtime) for debugging purposes."))
#if WITH_EDITOR
			.SetGroup(WorkspaceMenu::GetMenuStructure().GetDeveloperToolsDebugCategory())
			.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "ShowFlagsMenu.Grid"))
#endif
			;
	}

	void UnregisterNomadTabSpawner() { FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(GTabName); }

	void TryInvokeTab() { FGlobalTabmanager::Get()->TryInvokeTab(GTabName); }

	template <typename ExitClass>
	bool ObjectMatchesClassSearchString(const UObject* Object, const FString& SearchName)
	{
		if (!IsValid(Object))
			return false;

		auto* Class = Object->GetClass();
		// Iterate through all parent classes to find a match
		while (Class != UStruct::StaticClass() && Class != UClass::StaticClass() && Class != ExitClass::StaticClass())
		{
			if (Class->GetName().Equals(SearchName, ESearchCase::IgnoreCase))
				return true;
			Class = Class->GetSuperClass();
		}

		return false;
	}

	bool FActorQuery::MatchesActor(const AActor* Actor) const
	{
		if (!IsValid(Actor))
			return false;

		bool bAtLeastOneFilterActive = false;

		const FString ActorName = Actor->GetActorNameOrLabel();
		if (!NameFilter.IsEmpty())
		{
			bAtLeastOneFilterActive = true;
			if (!ActorName.Contains(NameFilter))
				return false;
		}

		if (!NameRegexPattern.IsEmpty())
		{
			bAtLeastOneFilterActive = true;
			if (!OUU::Runtime::RegexUtils::MatchesRegex(NameRegexPattern, ActorName))
				return false;
		}

		if (ActorClassName.IsEmpty() == false)
		{
			bAtLeastOneFilterActive = true;
			if (!ObjectMatchesClassSearchString<AActor>(Actor, ActorClassName))
				return false;
		}

		if (ComponentClassName.IsEmpty() == false)
		{
			bAtLeastOneFilterActive = true;
			bool bAtLeastOneComponentMatches = false;
			for (auto& Component : Actor->GetComponents())
			{
				if (ObjectMatchesClassSearchString<UActorComponent>(Component, ComponentClassName))
				{
					bAtLeastOneComponentMatches = true;
					break;
				}
			}
			if (!bAtLeastOneComponentMatches)
				return false;
		}

		if (!ActorTagQuery.IsEmpty())
		{
			bAtLeastOneFilterActive = true;
			if (UAbilitySystemComponent* AbilitySystemComponent =
					Actor->FindComponentByClass<UAbilitySystemComponent>())
			{
				FGameplayTagContainer OwnedTags;
				AbilitySystemComponent->GetOwnedGameplayTags(OUT OwnedTags);
				if (!ActorTagQuery.Matches(OwnedTags))
					return false;
			}
			else
			{
				return false;
			}
		}

		return bAtLeastOneFilterActive;
	}

	FActorQuery::FResult FActorQuery::ExecuteQuery(UWorld* World) const
	{
		FResult ResultList;
		if (!IsValid(World))
			return ResultList;

		for (AActor* Actor : TActorRange<AActor>(World))
		{
			if (!IsValid(Actor))
				continue;

			if (MatchesActor(Actor))
			{
				ResultList.Actors.Add(Actor);
			}
		}
		return ResultList;
	}

	//------------------------------------------------------------------------
	// Console command
	//------------------------------------------------------------------------

	static FAutoConsoleCommand OpenActorMapCommand(
		TEXT("ouu.Debug.OpenActorMap"),
		TEXT("Open an actor map for the current world (game or editor)"),
		FConsoleCommandDelegate::CreateStatic(TryInvokeTab));

} // namespace OUU::Developer::ActorMapWindow

UE_DISABLE_OPTIMIZATION
