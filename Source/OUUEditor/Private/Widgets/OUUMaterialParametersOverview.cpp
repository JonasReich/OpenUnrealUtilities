// Copyright (c) 2022 Jonas Reich

#include "Widgets/OUUMaterialParametersOverview.h"

#include "Curves/CurveLinearColor.h"
#include "EdGraph/EdGraph.h"
#include "IContentBrowserSingleton.h"
#include "IDetailTreeNode.h"
#include "IMaterialEditor.h"
#include "LogOpenUnrealUtilities.h"
#include "MaterialEditorUtilities.h"
#include "MaterialPropertyHelpers.h"
#include "Materials/Material.h"
#include "PropertyCustomizationHelpers.h"
#include "Toolkits/ToolkitManager.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/OUUMaterialParametersOverivew_EditorObject.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STableRow.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"

namespace OUU::Editor::Private::MaterialParametersOverview
{
	namespace Font
	{
		auto GetCategory() { return FEditorStyle::GetFontStyle(TEXT("DetailsView.CategoryFontStyle")); }
		auto GetNormal() { return FEditorStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")); }
		auto GetBold() { return FEditorStyle::GetFontStyle(TEXT("PropertyWindow.BoldFont")); }
	} // namespace Font

	namespace Parameter
	{
		template <typename ExpressionType>
		void FillInExpressions()
		{
			TArray<ExpressionType*> Expressions;
			auto* EditorObject = GetMutableDefault<UOUUMaterialParametersOverivew_EditorObject>();
			EditorObject->TargetMaterial->GetAllExpressionsInMaterialAndFunctionsOfType<ExpressionType>(
				OUT Expressions);
			for (const auto Parameter : EditorObject->Parameters)
			{
				const FName NameToFind = Parameter->Info.Name;
				ExpressionType** ExpressionPtr = Expressions.FindByPredicate(
					[NameToFind](auto* Expression) -> bool { return Expression->ParameterName == NameToFind; });
				ExpressionType* Expression = ExpressionPtr ? *ExpressionPtr : nullptr;
				if (!Expression)
					continue;

				Parameter->Expression = Expression;

				const UObject* Outer = Expression->GetOuter();
				if (Outer->IsA<UMaterialInterface>())
				{
					Parameter->Source = FSortedMaterialParameter::ESource::Material;
				}
				else if (Outer->IsA<UMaterialFunctionInterface>())
				{
					Parameter->Source = FSortedMaterialParameter::ESource::MaterialFunction;
				}
			}
		}

		void RegenerateList(FString FilterString)
		{
			auto* EditorObject = GetMutableDefault<UOUUMaterialParametersOverivew_EditorObject>();
			auto* TargetMaterial = EditorObject->TargetMaterial;
			auto& Parameters = EditorObject->Parameters;

			Parameters.Empty();

			if (!TargetMaterial)
				return;

			// Ensure all cached data is up-to-date before looping over parameters
			TargetMaterial->UpdateCachedExpressionData();

			// Loop through all types of parameters for this material and add them to the parameter arrays.
			TArray<FMaterialParameterInfo> ParameterInfos;
			TArray<FGuid> Guids;

			auto FillParameters = [&]() {
				for (int32 ParameterIdx = 0; ParameterIdx < ParameterInfos.Num(); ParameterIdx++)
				{
					auto& ParameterInfo = ParameterInfos[ParameterIdx];
					int32 SortPriority = 0;
					TargetMaterial->GetParameterSortPriority(ParameterInfo.Name, SortPriority);

					if (FilterString.IsEmpty() || ParameterInfo.Name.ToString().Contains(FilterString))
					{
						Parameters.Add(MakeShared<FSortedMaterialParameter>(ParameterInfo, SortPriority));
					}
				}
			};

			TargetMaterial->GetAllVectorParameterInfo(OUT ParameterInfos, OUT Guids);
			FillParameters();
			TargetMaterial->GetAllScalarParameterInfo(OUT ParameterInfos, OUT Guids);
			FillParameters();
			TargetMaterial->GetAllTextureParameterInfo(ParameterInfos, OUT Guids);
			FillParameters();
			TargetMaterial->GetAllFontParameterInfo(OUT ParameterInfos, OUT Guids);
			FillParameters();
			TargetMaterial->GetAllMaterialLayersParameterInfo(OUT ParameterInfos, OUT Guids);
			FillParameters();
			TargetMaterial->GetAllStaticSwitchParameterInfo(OUT ParameterInfos, OUT Guids);
			FillParameters();
			TargetMaterial->GetAllStaticComponentMaskParameterInfo(OUT ParameterInfos, OUT Guids);
			FillParameters();

			FillInExpressions<UMaterialExpressionTextureSampleParameter>();
			FillInExpressions<UMaterialExpressionParameter>();

			Parameters.Sort();
		}

		void JumpToExpression(FSortedMaterialParameter& ParameterData)
		{
			auto* Outer = ParameterData.Expression->GetOuter();

			switch (ParameterData.Source)
			{
			case FSortedMaterialParameter::ESource::Material:
			{
				FMaterialEditorUtilities::OnOpenMaterial(Outer);
				break;
			}
			case FSortedMaterialParameter::ESource::MaterialFunction:
			{
				FMaterialEditorUtilities::OnOpenFunction(Outer);
				break;
			}
			default:
			{
				UE_LOG(LogOpenUnrealUtilities, Warning, TEXT("Cannot open asset because outer is undefined"));
			}
			}

			TSharedPtr<IToolkit> FoundAssetEditor = FToolkitManager::Get().FindEditorForAsset(Outer);
			TSharedPtr<IMaterialEditor> EditorPtr = StaticCastSharedPtr<IMaterialEditor>(FoundAssetEditor);

			if (EditorPtr.IsValid())
			{
				EditorPtr->FocusWindow();
				EditorPtr->JumpToExpression(ParameterData.Expression);
			}
		}
	} // namespace Parameter

	namespace Widgets
	{
		TSharedRef<SWidget> Splitter(
			FMaterialTreeColumnSizeData* ColumnSizeData,
			TSharedRef<SWidget> LeftWidget,
			TSharedRef<SWidget> RightWidget)
		{
			// clang-format off
			return SNew(SSplitter)
				.Style(FEditorStyle::Get(), "DetailsView.Splitter")
				.PhysicalSplitterHandleSize(1.0f)
				.HitDetectionSplitterHandleSize(5.0f)
				+ SSplitter::Slot()
				.Value(ColumnSizeData->LeftColumnWidth)
				.OnSlotResized(ColumnSizeData->OnWidthChanged)
				[
					LeftWidget
				]
				+ SSplitter::Slot()
				.Value(ColumnSizeData->RightColumnWidth)
				.OnSlotResized(ColumnSizeData->OnWidthChanged)
				[
					RightWidget
				];
			// clang-format on
		}

		class SMaterialParametersOverviewPanel : public SCompoundWidget
		{
		public:
			SLATE_BEGIN_ARGS(SMaterialParametersOverviewPanel)
				{
				}
			SLATE_END_ARGS()

			void Construct(const FArguments& InArgs);

		private:
			TSharedPtr<class SMaterialParametersOverviewList> NestedTree;
			TSharedPtr<class SScrollBar> ExternalScrollbar;
			TSharedPtr<SObjectPropertyEntryBox> AssetPicker;
			TSharedPtr<SBox> MainWidget;

			FString SelectedAssetPath;
			FString GetSelectedAssetPath() const { return SelectedAssetPath; }

			FString FilterString;
			FText GetFilterText() const { return FText::FromString(FilterString); }

			FMaterialTreeColumnSizeData ColumnSizeData;
			float ColumnWidth = 0.5f;
			float OnGetLeftColumnWidth() const { return 1.0f - ColumnWidth; }
			float OnGetRightColumnWidth() const { return ColumnWidth; }
			void OnSetColumnWidth(float InWidth) { ColumnWidth = InWidth; }

			void Refresh();

			void HandleFilterTextChanged(const FText& Text);

			void HandleAssetSelected(const FAssetData& InAssetData);
		};

		class SMaterialParametersOverviewListItem : public STableRow<TSharedPtr<FSortedParamData>>
		{
		public:
			SLATE_BEGIN_ARGS(SMaterialParametersOverviewListItem)
				: _ParameterData(nullptr),
				_ColumnSizeData(nullptr)
			{
			}
				SLATE_ARGUMENT(TSharedPtr<FSortedMaterialParameter>, ParameterData)
				SLATE_ARGUMENT(FMaterialTreeColumnSizeData*, ColumnSizeData)
			SLATE_END_ARGS()

			void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTableView);

		private:
			const FSlateBrush* GetBorderImage() const;

			FReply OnParameterButtonClicked() const;

		private:
			/** The node info to build the tree view row from. */
			TSharedPtr<FSortedMaterialParameter> ParameterData;

			FMaterialTreeColumnSizeData* ColumnSizeData = nullptr;
		};

		class SMaterialParametersOverviewList : public SListView<TSharedPtr<FSortedMaterialParameter>>
		{
		public:
			SLATE_BEGIN_ARGS(SMaterialParametersOverviewList)
				:_Owner(nullptr),
				_Scrollbar(nullptr),
				_ColumnSizeData(nullptr)
			{
			}
				SLATE_ARGUMENT(TSharedPtr<SMaterialParametersOverviewPanel>, Owner)
				SLATE_ARGUMENT(TSharedPtr<SScrollBar>, Scrollbar)
				SLATE_ARGUMENT(FMaterialTreeColumnSizeData*, ColumnSizeData)
			SLATE_END_ARGS()

			void Construct(const FArguments& InArgs);

			bool HasAnyItems() const { return ItemsSource->Num() > 0; }

		private:
			TSharedRef<ITableRow> GenerateRowWidget(
				TSharedPtr<FSortedMaterialParameter> Item,
				const TSharedRef<STableViewBase>& OwnerTable) const;

			TWeakPtr<SMaterialParametersOverviewPanel> Owner;
			FMaterialTreeColumnSizeData* ColumnSizeData = nullptr;
		};

		////////////////////////////

		void SMaterialParametersOverviewPanel::Construct(const FArguments& InArgs)
		{
			ColumnSizeData.LeftColumnWidth =
				TAttribute<float>(this, &SMaterialParametersOverviewPanel::OnGetLeftColumnWidth);
			ColumnSizeData.RightColumnWidth =
				TAttribute<float>(this, &SMaterialParametersOverviewPanel::OnGetRightColumnWidth);
			ColumnSizeData.OnWidthChanged =
				SSplitter::FOnSlotResized::CreateSP(this, &SMaterialParametersOverviewPanel::OnSetColumnWidth);

			ExternalScrollbar = SNew(SScrollBar);

			NestedTree = SNew(SMaterialParametersOverviewList)
							 .Owner(SharedThis(this))
							 .Scrollbar(ExternalScrollbar)
							 .ColumnSizeData(&ColumnSizeData);

			AssetPicker = SNew(SObjectPropertyEntryBox)
							  .ObjectPath(this, &SMaterialParametersOverviewPanel::GetSelectedAssetPath)
							  .AllowedClass(UMaterial::StaticClass())
							  .OnObjectChanged(this, &SMaterialParametersOverviewPanel::HandleAssetSelected)
							  .AllowClear(true)
							  .DisplayUseSelected(true)
							  .DisplayBrowse(true)
							  .DisplayThumbnail(true);

			// clang-format off
			this->ChildSlot
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					AssetPicker.ToSharedRef()
				]
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SEditableTextBox)
					.HintText(INVTEXT("Filter..."))
					.OnTextChanged(this, &SMaterialParametersOverviewPanel::HandleFilterTextChanged)
					.Text(this, &SMaterialParametersOverviewPanel::GetFilterText)
					.ClearKeyboardFocusOnCommit(false)
				]
				+ SVerticalBox::Slot()
				[
					SAssignNew(MainWidget, SBox)
				]
			];
			// clang-format on

			Refresh();
		}

		void SMaterialParametersOverviewPanel::Refresh()
		{
			NestedTree->RequestListRefresh();

			if (!NestedTree->HasAnyItems())
			{
				MainWidget->SetContent(
					SNew(SBorder)
						.BorderImage(FEditorStyle::GetBrush("MaterialInstanceEditor.StackBody"))
						.Padding(FMargin(4.0f))[SNew(STextBlock).Text(INVTEXT("Connect a parameter to see it here."))]);
				return;
			}

			// clang-format off
			MainWidget->SetContent(
				SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush("DetailsView.CategoryTop_Hovered"))
				.Padding(FMargin(4.0f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						Widgets::Splitter(&ColumnSizeData,
							// left
							SNew(STextBlock)
							.Text(INVTEXT("Parameters"))
							.Font(Font::GetCategory())
							.ShadowOffset(FVector2D(1.0f, 1.0f)),
							// right
							SNew(STextBlock)
							.Text(INVTEXT("External Source"))
							.Font(Font::GetCategory())
							.ShadowOffset(FVector2D(1.0f, 1.0f))
						)
					]
					+ SVerticalBox::Slot()
					.Padding(FMargin(3.0f, 2.0f, 3.0f, 3.0f))
					[
						SNew(SBorder)
						.BorderImage(FEditorStyle::GetBrush("DetailsView.CategoryTop"))
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().HAlign(HAlign_Fill)
							[
								SNew(SWidgetSwitcher)
								.WidgetIndex_Lambda([this]() -> int32{ return NestedTree && NestedTree->HasAnyItems() ? 1 : 0; })
								+ SWidgetSwitcher::Slot()
								[
									SNew(STextBlock)
									.Text(INVTEXT("Add parameters to see them here."))
								]
								+ SWidgetSwitcher::Slot()
								[
									NestedTree.ToSharedRef()
								]
							]
							+ SHorizontalBox::Slot()
							.HAlign(HAlign_Right)
							.AutoWidth()
							[
								SNew(SBox).WidthOverride(16.0f)
								[
									ExternalScrollbar.ToSharedRef()
								]
							]
						]
					]
				]
			);
			// clang-format on
		}

		void SMaterialParametersOverviewPanel::HandleFilterTextChanged(const FText& Text)
		{
			FilterString = Text.ToString();
			Parameter::RegenerateList(FilterString);
			Refresh();
		}

		void SMaterialParametersOverviewPanel::HandleAssetSelected(const FAssetData& InAssetData)
		{
			auto* EditorObject = GetMutableDefault<UOUUMaterialParametersOverivew_EditorObject>();
			SelectedAssetPath = InAssetData.ObjectPath.ToString();
			if (auto* Material = Cast<UMaterial>(InAssetData.GetAsset()))
			{
				EditorObject->TargetMaterial = Material;
			}
			Parameter::RegenerateList(FilterString);
			Refresh();
		}

		void SMaterialParametersOverviewListItem::Construct(
			const FArguments& InArgs,
			const TSharedRef<STableViewBase>& OwnerTableView)
		{
			ParameterData = InArgs._ParameterData;
			ColumnSizeData = InArgs._ColumnSizeData;

			// clang-format off
			this->ChildSlot[
				SNew(SBorder)
				.Padding(0.0f)
				.BorderImage(this, &SMaterialParametersOverviewListItem::GetBorderImage)
				[
					Widgets::Splitter(ColumnSizeData,
						// left
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.FillWidth(1.f)
						.Padding(3.f, 0.f)
						[
							SNew(STextBlock)
							.Text(FText::FromName(ParameterData->Info.Name))
						]
						+SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(3.f, 0.f)
						[
							SNew(SButton)
							.Text(INVTEXT("Open"))
							.OnClicked(this, &SMaterialParametersOverviewListItem::OnParameterButtonClicked)
						],
						// right
						SNew(STextBlock)
						.Text(ParameterData->Source == FSortedMaterialParameter::ESource::MaterialFunction ? FText::FromString(ParameterData->Expression->GetOuter()->GetName()) : FText::GetEmpty())
					)
				]
			];
			// clang-format on

			STableRow<TSharedPtr<FSortedParamData>>::ConstructInternal(
				STableRow::FArguments()
					.Style(FEditorStyle::Get(), "DetailsView.TreeView.TableRow")
					.ShowSelection(false),
				OwnerTableView);
		}

		const FSlateBrush* SMaterialParametersOverviewListItem::GetBorderImage() const
		{
			return IsHovered() ? FEditorStyle::GetBrush("DetailsView.CategoryMiddle_Hovered")
							   : FEditorStyle::GetBrush("DetailsView.CategoryMiddle");
		}

		FReply SMaterialParametersOverviewListItem::OnParameterButtonClicked() const
		{
			Parameter::JumpToExpression(*ParameterData.Get());
			return FReply::Handled();
		}

		void SMaterialParametersOverviewList::Construct(const FArguments& InArgs)
		{
			Owner = InArgs._Owner;
			ColumnSizeData = InArgs._ColumnSizeData;

			SListView<TSharedPtr<FSortedMaterialParameter>>::Construct(
				SListView::FArguments()
					.ListItemsSource(&GetMutableDefault<UOUUMaterialParametersOverivew_EditorObject>()->Parameters)
					.SelectionMode(ESelectionMode::None)
					.OnGenerateRow(this, &SMaterialParametersOverviewList::GenerateRowWidget)
					.ExternalScrollbar(InArgs._Scrollbar));
		}

		TSharedRef<ITableRow> SMaterialParametersOverviewList::GenerateRowWidget(
			TSharedPtr<FSortedMaterialParameter> Item,
			const TSharedRef<STableViewBase>& OwnerTable) const
		{
			return SNew(SMaterialParametersOverviewListItem, OwnerTable)
				.ParameterData(Item)
				.ColumnSizeData(ColumnSizeData);
		}

		TSharedRef<SDockTab> SpawnTab(const FSpawnTabArgs& Args)
		{
			// clang-format off
			return SNew(SDockTab)
			.Icon(FEditorStyle::GetBrush("LevelEditor.Tabs.Details"))
			.Label(INVTEXT("Material Paramters Overview (OUU)"))
			[
				SNew(SBox)
				[
					SNew(SMaterialParametersOverviewPanel)
				]
			];
			// clang-format on
		}
	} // namespace Widgets

} // namespace OUU::Editor::Private::MaterialParametersOverview

namespace OUU::Editor::MaterialParametersOverview
{
	const FName TabName = TEXT("OUU.MaterialParametersOverview");

	void RegisterNomadTabSpawner()
	{
		FGlobalTabmanager::Get()
			->RegisterNomadTabSpawner(
				TabName,
				FOnSpawnTab::CreateStatic(&Private::MaterialParametersOverview::Widgets::SpawnTab))
			.SetDisplayName(INVTEXT("Material Paramters Overview (OUU)"))
			.SetTooltipText(INVTEXT("Search and navigate to source of material parameters"))
			.SetGroup(WorkspaceMenu::GetMenuStructure().GetDeveloperToolsMiscCategory())
			.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Details"));
	}

	void UnregisterNomadTabSpawner() { FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TabName); }

} // namespace OUU::Editor::MaterialParametersOverview
