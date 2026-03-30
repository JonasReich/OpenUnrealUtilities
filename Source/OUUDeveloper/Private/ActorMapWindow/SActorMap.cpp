// Copyright (c) 2026 Jonas Reich & Contributors

#include "SActorMap.h"

#include "ActorMapWindow/OUUActorMapSettings.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "EngineUtils.h"
#include "OUUActorMapStyle.h"
#include "SActorLocationOverlay.h"
#include "SActorQueryRow.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/SVectorInputBox.h"
#include "Widgets/Layout/SScaleBox.h"

#if WITH_EDITOR
	#include "LevelEditorViewport.h"
	#include "WorldPartition/WorldPartition.h"
	#include "WorldPartition/WorldPartitionActorDescInstance.h"
	#include "WorldPartition/WorldPartitionHelpers.h"
	#include "WorldPartition/WorldPartitionMiniMap.h"
	#include "WorldPartition/WorldPartitionMiniMapHelper.h"
#endif

namespace OUU::Developer::ActorMapWindow
{
	namespace Private
	{
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

	} // namespace Private

	FText SActorMap::GetTitleText() const
	{
		return FText::FromString(FString::Printf(
			TEXT("OUU Actor Map (%s) [%s]"),
			*TargetWorld->GetName(),
			Private::GetWorldTypeString(TargetWorld->WorldType)));
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
		ActorQueries.Add(MakeShared<FOUUActorMapQuery>());
		RebuildQueryList();
	}

	void SActorMap::ClearQueries()
	{
		ActorQueries.Empty();
		RebuildQueryList();
	}

	void SActorMap::ResetQueries()
	{
		auto& Settings = *GetDefault<UOUUActorMapSettings>();
		ActorQueries.Reset();
		for (auto& DefaultQuery : Settings.DefaultQueries)
		{
			ActorQueries.Add(MakeShared<FOUUActorMapQuery>(DefaultQuery));
		}
		RebuildQueryList();
	}

	void SActorMap::RebuildQueryList()
	{
		QueryResults.Reset();
		if (ActorQueryListWidget.IsValid())
		{
			for (int32 i = 0; ActorQueries.IsValidIndex(i); ++i)
			{
				ActorQueries[i]->QueryColor = Private::DefaultColors[i % Private::DefaultColors.Num()];
			}
			ActorQueryListWidget->RebuildList();
		}
	}

	void SActorMap::WriteQueriesToDefaultConfig()
	{
		auto& Settings = *GetMutableDefault<UOUUActorMapSettings>();
		Settings.DefaultQueries.Reset();
		for (auto QueryPtr : ActorQueries)
		{
			Settings.DefaultQueries.Add(*QueryPtr);
		}
		Settings.TryUpdateDefaultConfigFile();
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
		#if WITH_EDITOR
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			DetailsColumns.MakeSimpleDetailsSplitter(
				INVTEXT("Query Unloaded Actors"),
				INVTEXT("If to query world partition actor descriptions instead of loaded actors. If this is checked, some query fields won't be available"),
				SNew(SCheckBox)
					.IsChecked(this, &SActorMap::GetQueryWorldPartitionCheckBoxState)
					.OnCheckStateChanged(this, &SActorMap::OnQueryWorldPartitionCheckBoxStateChanged)
			)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			DetailsColumns.MakeSimpleDetailsSplitter(
				INVTEXT(""),
				INVTEXT(""),
				SNew(SButton)
				.Text(INVTEXT("Refresh"))
					.OnClicked_Lambda([this]()
					{
						// Reset the query results and set status flag to false.
						// This will trigger one full pass over the WP distributed over multiple ticks.
						QueryResults.Reset();
						WorldPartitionCellIndexPerQuery.Reset();
						return FReply::Handled();
					})
			)
		]
		#endif
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
				INVTEXT("Show Names"),
				INVTEXT("Draw name labels for each actor?"),
				SNew(SCheckBox)
					.IsChecked(this, &SActorMap::GetDrawLabelsCheckBoxState)
					.OnCheckStateChanged(this, &SActorMap::OnDrawLabelsCheckBoxStateChanged)
			)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			DetailsColumns.MakeSimpleDetailsSplitter(
				INVTEXT("Live Background"),
				INVTEXT("Draw a live updating scene capture of the world?"),
				SNew(SCheckBox)
					.IsChecked(this, &SActorMap::GetDrawSceneCaptureCheckBoxState)
					.OnCheckStateChanged(this, &SActorMap::OnDrawSceneCaptureCheckBoxStateChanged)
			)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			DetailsColumns.MakeSimpleDetailsSplitter(
				INVTEXT("Show Distance"),
				INVTEXT("Show a distance circle overlay to judge approximate distance from the center?"),
				SNew(SCheckBox)
					.IsChecked(this, &SActorMap::GetDrawDistanceCheckBoxState)
					.OnCheckStateChanged(this, &SActorMap::OnDrawDistanceCheckBoxStateChanged)
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
					.Text(INVTEXT("Add Query"))
					.OnPressed(this, &SActorMap::AddActorQuery)
			]
			+ SHorizontalBox::Slot()
			[
				SNew(SButton)
					.Text(INVTEXT("Clear Queries"))
					.OnPressed(this, &SActorMap::ClearQueries)
			]
			+ SHorizontalBox::Slot()
			[
				SNew(SButton)
					.Text(INVTEXT("Reset to Default"))
					.OnPressed(this, &SActorMap::ResetQueries)
			]
			+ SHorizontalBox::Slot()
			[
				SNew(SButton)
					.Text(INVTEXT("Save Default"))
					.ToolTipText(INVTEXT("Save current queries as default to the DefaultGame.ini"))
					.OnPressed(this, &SActorMap::WriteQueriesToDefaultConfig)
			]
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		[
			SAssignNew(ActorQueryListWidget, SListView<TSharedPtr<FOUUActorMapQuery>>)
				.ListItemsSource(&ActorQueries)
				// Do not make the items selectable for now. We don't support any operations based on selection yet.
				.SelectionMode(ESelectionMode::None)
				.OnGenerateRow(this, &SActorMap::OnGenerateActorQueryRow)
		]
	];
		// clang-format on
	}

	TSharedRef<ITableRow> SActorMap::OnGenerateActorQueryRow(
		TSharedPtr<FOUUActorMapQuery> InItem,
		const TSharedRef<STableViewBase>& OwnerTable)
	{
		return SNew(SActorQueryRow, OwnerTable, InItem)
			.ColumnSizeData(&DetailsColumns)
			.EnableComponentFilters_Lambda([this]() {
#if WITH_EDITOR
				return bQueryWorldPartition == false;
#else
				return true;
#endif
			})
			.OnDeleteClicked_Lambda([this, WeakItem = InItem.ToWeakPtr()]() {
				if (auto SharedItem = WeakItem.Pin())
				{
					ActorQueries.Remove(SharedItem);
					RebuildQueryList();
				}
				return FReply::Handled();
			});
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
			.Visibility_Lambda([this]()->EVisibility{ return ShowFlags & (EShowFlags::SceneCapture) ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed; })
			.Content()
			[
				SNew(SImage)
                .Image(&MapBrush)
			]
		]
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.Clipping(EWidgetClipping::ClipToBounds)
			[	
				SNew(SActorLocationOverlay, TargetWorld.Get())
					.ActorQueries(&ActorQueries)
					.MapSize(this, &SActorMap::GetOrthoWidth)
					.ShowFlags(this, &SActorMap::GetShowFlags)
					.ReferencePosition(this, &SActorMap::GetReferencePosition)
			]
		];
		// clang-format on
	}

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
		ChildSlot[ConstructWidget()];
		ResetQueries();
	}

	void SActorMap::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
	{
		Super::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

		// Magic number for delta time that we ignore.
		// I only want to update the actor map if the DT is smaller than a second.
		// Otherwise, there's bigger fish to fry.
		constexpr float MagicDeltaTimeLimit = 1000;
		if (!ensure(TickRate > 0) || InDeltaTime > MagicDeltaTimeLimit)
			return;

		UpdateQueryResults();

		AccumulatedDeltaTime += InDeltaTime;

		if (AccumulatedDeltaTime >= TickRate)
		{
			while (AccumulatedDeltaTime >= TickRate)
			{
				AccumulatedDeltaTime -= TickRate;
			}

			UpdateSceneCapture();
		}
	}

	void SActorMap::UpdateQueryResults()
	{
		UWorld* World = TargetWorld.Get();
		if (!IsValid(World))
			return;

		if (ActorQueries.Num() <= 0)
			return;

		if (ActorQueries.Num() != QueryResults.Num())
		{
			QueryResults.SetNum(ActorQueries.Num());
			WorldPartitionCellIndexPerQuery.SetNum(ActorQueries.Num());
			for (int32 i = 0; i < ActorQueries.Num(); ++i)
			{
				ActorQueries[i]->CachedResult = &QueryResults[i];
				WorldPartitionCellIndexPerQuery[i] = 0;
			}
		}

		uint32 QueryIndex = GFrameNumber % ActorQueries.Num();
		auto& Query = ActorQueries[QueryIndex];
		auto& CurrentQueryResults = QueryResults[QueryIndex];

		if (Query.IsValid())
		{
#if WITH_EDITOR
			if (bQueryWorldPartition)
			{
				if (auto* WorldPartition = GetTargetWorld()->GetWorldPartition())
				{
					constexpr uint32 NumSubdivisionsByAxis = 20;
					constexpr uint32 NumSubdivisions = NumSubdivisionsByAxis * NumSubdivisionsByAxis;

					if (WorldPartitionCellIndexPerQuery[QueryIndex] == NumSubdivisions)
					{
						return;
					}

					uint32& SubdivisionIdx = WorldPartitionCellIndexPerQuery[QueryIndex];

					uint32 Subdivision_X = SubdivisionIdx / NumSubdivisionsByAxis;
					uint32 Subdivision_Y = SubdivisionIdx % NumSubdivisionsByAxis;

					auto TotalWorldBounds = WorldPartition->GetEditorWorldBounds();
					auto VerticalExtent = TotalWorldBounds.GetExtent().Z;

					FVector2D Subdivisions_Min = FVector2D(ReferencePosition.X, ReferencePosition.Y)
						- FVector2D(OrthoWidth / 2.f, OrthoWidth / 2.f);
					float SubDivisionWidth = OrthoWidth / NumSubdivisionsByAxis;
					FVector2D CurrentSubdivision_Min = Subdivisions_Min
						+ FVector2D(SubDivisionWidth * Subdivision_X, SubDivisionWidth * Subdivision_Y);
					FVector2D CurrentSubdivision_Max =
						CurrentSubdivision_Min + FVector2D(SubDivisionWidth, SubDivisionWidth);

					FBox SubdivisionBox = FBox(
						FVector{CurrentSubdivision_Min.X, CurrentSubdivision_Min.Y, -VerticalExtent},
						FVector{CurrentSubdivision_Max.X, CurrentSubdivision_Max.Y, VerticalExtent});

					FWorldPartitionHelpers::ForEachIntersectingActorDescInstance(
						WorldPartition,
						SubdivisionBox,
						[&](const FWorldPartitionActorDescInstance* ActorDesc) -> bool {
							if (ActorDesc->IsLoaded())
							{
								auto* Actor = ActorDesc->GetActor();
								if (Query->MatchesActor(Actor))
								{
									FString String =
										(ShowFlags & EShowFlags::Labels) ? ActorDesc->GetActorLabelString() : FString();
									CurrentQueryResults.Add({Actor->GetActorLocation(), MoveTemp(String)});
								}
							}
							else if (Query->MatchesActorDescription(*ActorDesc))
							{
								FString String =
									(ShowFlags & EShowFlags::Labels) ? ActorDesc->GetActorLabelString() : FString();
								CurrentQueryResults.Add({ActorDesc->GetEditorBounds().GetCenter(), MoveTemp(String)});
							}
							return true;
						});

					++SubdivisionIdx;
				}
			}
			else
#endif
			{
				// non WP query results are reset every tick
				CurrentQueryResults.Reset();
				for (auto* Actor : TActorRange<AActor>(World))
				{
					if (IsValid(Actor) && Query->MatchesActor(Actor))
					{
						FString String = (ShowFlags & EShowFlags::Labels) ? Actor->GetActorNameOrLabel() : FString();
						CurrentQueryResults.Add({Actor->GetActorLocation(), MoveTemp(String)});
					}
				}
			}
		}
	}

	void SActorMap::UpdateSceneCapture()
	{
		if (SceneCaptureActor.IsValid() && ShowFlags & EShowFlags::SceneCapture)
		{
			LocalCameraLocation = FVector::ZeroVector;
			if (bFollowCamera)
			{
				bool bSetLocalCameraLocation = false;
				if (const auto* LocalPlayerController = TargetWorld->GetFirstPlayerController())
				{
					if (const APlayerCameraManager* Camera = LocalPlayerController->PlayerCameraManager.Get())
					{
						bSetLocalCameraLocation = true;
						LocalCameraLocation = Camera->GetCameraLocation();
					}
				}
				if (!bSetLocalCameraLocation)
				{
#if WITH_EDITOR
					for (const FLevelEditorViewportClient* LevelVC : GEditor->GetLevelViewportClients())
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
	}

	UWorld* SActorMap::GetDynamicTargetWorld() const
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

	void SActorMap::InitializeForWorld(UWorld* InTargetWorld)
	{
		check(IsValid(InTargetWorld));

		TargetWorld = InTargetWorld;

#if WITH_EDITOR
		if (InTargetWorld->WorldType == EWorldType::Editor)
		{
			if (AWorldPartitionMiniMap* WorldMiniMap =
					FWorldPartitionMiniMapHelper::GetWorldPartitionMiniMap(InTargetWorld))
			{
				// for editor worlds that have a world partition minimap:
				// default to a much wider frame matching the minimap, disable dynamic scene capture and labels.

				ShowFlags &= ~EShowFlags::SceneCapture;
				ShowFlags &= ~EShowFlags::Labels;

				ReferencePosition = WorldMiniMap->GetMiniMapWorldBounds().GetCenter();
				ReferencePosition.Z = 10000;
				auto MinimapExtent = WorldMiniMap->GetMiniMapWorldBounds().GetExtent();
				// Extent is a half extent, but ortho width is full width
				OrthoWidth = FMath::Max(MinimapExtent.X, MinimapExtent.Y) * 2.f;
			}
		}
#endif

		// Look down
		const FRotator Direction(-90, -90, 0);

		FActorSpawnParameters Params = FActorSpawnParameters();
		// the scene capture actor should never be saved to the level or be visible to the user
		Params.ObjectFlags |= RF_Transient;

#if WITH_EDITOR
		Params.bCreateActorPackage = false;
		Params.OverridePackage = nullptr;
		Params.bHideFromSceneOutliner = true;
#endif

		SceneCaptureActor = TargetWorld->SpawnActor<ASceneCapture2D>(ReferencePosition, Direction, Params);
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

	TSharedRef<SWidget> SActorMap::ConstructWidget()
	{
		// clang-format off
		return SNew(SBorder).BorderImage(&Style::DarkGrey).Content()[SNew(SSplitter)
		+ SSplitter::Slot()
		.SizeRule(SSplitter::ESizeRule::FractionOfParent)
		.Value(MainColumns.LeftColumnWidth)
		.OnSlotResized(MainColumns.OnWidthChanged)
		[
			SNew(SBorder)
				.Padding(2.f)
				.BorderImage(&Style::MediumGrey)
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
				.BorderImage(&Style::MediumGrey)
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

} // namespace OUU::Developer::ActorMapWindow
