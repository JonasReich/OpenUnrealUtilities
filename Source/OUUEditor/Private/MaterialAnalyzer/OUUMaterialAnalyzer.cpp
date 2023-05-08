// Copyright (c) 2022 Jonas Reich

#include "MaterialAnalyzer/OUUMaterialAnalyzer.h"

#include "Curves/CurveLinearColor.h"
#include "EdGraph/EdGraph.h"
#include "EditorStyleSet.h"
#include "IContentBrowserSingleton.h"
#include "IDetailTreeNode.h"
#include "IMaterialEditor.h"
#include "MaterialAnalyzer/OUUMaterialAnalyzer_EditorObject.h"
#include "MaterialAnalyzer/OUUMaterialAnalyzer_ParametersList.h"
#include "MaterialEditorUtilities.h"
#include "MaterialPropertyHelpers.h"
#include "Materials/Material.h"
#include "PropertyCustomizationHelpers.h"
#include "Slate/SplitterColumnSizeData.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STableRow.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"

namespace OUU::Editor::Private::MaterialAnalyzer
{
	namespace Font
	{
		auto GetCategory() { return FAppStyle::GetFontStyle(TEXT("DetailsView.CategoryFontStyle")); }
		auto GetNormal() { return FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")); }
		auto GetBold() { return FAppStyle::GetFontStyle(TEXT("PropertyWindow.BoldFont")); }
	} // namespace Font

	namespace Parameter
	{
		template <typename ExpressionType>
		void FillInExpressions()
		{
			TArray<ExpressionType*> Expressions;
			auto* EditorObject = GetMutableDefault<UOUUMaterialAnalyzer_EditorObject>();
			EditorObject->TargetMaterial->GetAllExpressionsInMaterialAndFunctionsOfType<ExpressionType>(
				OUT Expressions);
			for (const auto& Parameter : EditorObject->Parameters)
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
					Parameter->Source = FOUUMaterialAnalyzer_ParameterData::ESource::Material;
				}
				else if (Outer->IsA<UMaterialFunctionInterface>())
				{
					Parameter->Source = FOUUMaterialAnalyzer_ParameterData::ESource::MaterialFunction;
				}
			}
		}

		void RegenerateList(
			TSharedPtr<Widgets::SOUUMaterialAnalyzer_ParametersList> ParametersList,
			FString FilterString)
		{
			ParametersList->RequestListRefresh();

			auto* EditorObject = GetMutableDefault<UOUUMaterialAnalyzer_EditorObject>();
			auto* TargetMaterial = EditorObject->TargetMaterial;
			auto& Parameters = EditorObject->Parameters;

			Parameters.Empty();

			if (!TargetMaterial)
				return;

			// Ensure all cached data is up-to-date before looping over parameters
			TargetMaterial->UpdateCachedExpressionData();

			// Loop through all types of parameters for this material and add them to the parameter arrays.
			TArray<FMaterialParameterInfo> ParameterInfos;
			TArray<FGuid> ParameterIds;
			for (int32 ParameterTypeIndex = 0; ParameterTypeIndex < NumMaterialParameterTypes; ++ParameterTypeIndex)
			{
				const EMaterialParameterType ParameterType = StaticCast<EMaterialParameterType>(ParameterTypeIndex);
				TargetMaterial->GetAllParameterInfoOfType(ParameterType, ParameterInfos, ParameterIds);
				for (const FMaterialParameterInfo& ParameterInfo : ParameterInfos)
				{
					int32 SortPriority = 0;
					TargetMaterial->GetParameterSortPriority(ParameterInfo.Name, SortPriority);

					if (FilterString.IsEmpty() || ParameterInfo.Name.ToString().Contains(FilterString))
					{
						Parameters.Add(MakeShared<FOUUMaterialAnalyzer_ParameterData>(ParameterInfo, SortPriority));
					}
				}
			}

			FillInExpressions<UMaterialExpressionTextureSampleParameter>();
			FillInExpressions<UMaterialExpressionParameter>();

			Parameters.Sort();
		}

	} // namespace Parameter

	namespace Widgets
	{
		TSharedRef<SWidget> Splitter(
			TSharedPtr<FSplitterColumnSizeData> ColumnSizeData,
			TSharedRef<SWidget> LeftWidget,
			TSharedRef<SWidget> RightWidget)
		{
			// clang-format off
			return SNew(SSplitter)
				.Style(FAppStyle::Get(), "DetailsView.Splitter")
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

		TSharedRef<SWidget> ListWrapper(
			TSharedPtr<FSplitterColumnSizeData> ColumnSizeData,
			TSharedRef<SScrollBar> ScrollBar,
			FText LeftColumnLabel,
			FText RightColumnLabel,
			TSharedRef<SWidget> Content)
		{
			// clang-format off
			return SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("DetailsView.CategoryTop_Hovered"))
				.Padding(FMargin(4.0f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.Padding(FMargin(3.0f, 2.0f, 3.0f, 3.0f))
					.AutoHeight()
					[
						Widgets::Splitter(ColumnSizeData,
							// left
							SNew(STextBlock)
							.Text(LeftColumnLabel)
							.Font(Font::GetCategory())
							.ShadowOffset(FVector2D(1.0f, 1.0f)),
							// right
							SNew(STextBlock)
							.Text(RightColumnLabel)
							.Font(Font::GetCategory())
							.ShadowOffset(FVector2D(1.0f, 1.0f))
						)
					]
					+ SVerticalBox::Slot()
					.Padding(FMargin(3.0f, 2.0f, 3.0f, 3.0f))
					[
						SNew(SBorder)
						.BorderImage(FAppStyle::GetBrush("DetailsView.CategoryTop"))
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().HAlign(HAlign_Fill)
							[
								Content
							]
							+ SHorizontalBox::Slot()
							.HAlign(HAlign_Right)
							.AutoWidth()
							[
								SNew(SBox).WidthOverride(16.0f)
								[
									ScrollBar
								]
							]
						]
					]
				];
			// clang-format on
		}

		class SOUUMaterialAnalyzer : public SCompoundWidget
		{
		public:
			SLATE_BEGIN_ARGS(SOUUMaterialAnalyzer)
				{
				}
			SLATE_END_ARGS()

			void Construct(const FArguments& InArgs);

		private:
			TSharedPtr<SOUUMaterialAnalyzer_ParametersList> ParametersList;
			TSharedPtr<SScrollBar> ParametersListScrollbar;
			TSharedPtr<SObjectPropertyEntryBox> AssetPicker;
			TSharedPtr<SBox> ParameterListRoot;

			FString SelectedAssetPath;
			FString GetSelectedAssetPath() const { return SelectedAssetPath; }

			FString ParameterFilterString;
			FText GetParameterFilterText() const { return FText::FromString(ParameterFilterString); }
			void HandleParameterFilterTextChanged(const FText& Text);

			TSharedPtr<FSplitterColumnSizeData> ColumnSizeData;

			void HandleAssetSelected(const FAssetData& InAssetData);
			void RefreshParamtersList();
		};

		////////////////////////////

		void SOUUMaterialAnalyzer::Construct(const FArguments& InArgs)
		{
			ColumnSizeData = MakeShared<FSplitterColumnSizeData>();

			ParametersListScrollbar = SNew(SScrollBar);

			ParametersList = SNew(SOUUMaterialAnalyzer_ParametersList)
								 .Scrollbar(ParametersListScrollbar)
								 .ColumnSizeData(ColumnSizeData);

			AssetPicker = SNew(SObjectPropertyEntryBox)
							  .ObjectPath(this, &SOUUMaterialAnalyzer::GetSelectedAssetPath)
							  .AllowedClass(UMaterial::StaticClass())
							  .OnObjectChanged(this, &SOUUMaterialAnalyzer::HandleAssetSelected)
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
					.OnTextChanged(this, &SOUUMaterialAnalyzer::HandleParameterFilterTextChanged)
					.Text(this, &SOUUMaterialAnalyzer::GetParameterFilterText)
					.ClearKeyboardFocusOnCommit(false)
				]
				+ SVerticalBox::Slot()
				.FillHeight(2.f)
				[
					SAssignNew(ParameterListRoot, SBox)
				]
			];
			// clang-format on

			RefreshParamtersList();
		}

		void SOUUMaterialAnalyzer::RefreshParamtersList()
		{
			Parameter::RegenerateList(ParametersList, ParameterFilterString);

			if (!ParametersList->HasAnyItems())
			{
				ParameterListRoot->SetContent(
					SNew(SBorder)
						.BorderImage(FAppStyle::GetBrush("MaterialInstanceEditor.StackBody"))
						.Padding(FMargin(4.0f))[SNew(STextBlock).Text(INVTEXT("Connect a parameter to see it here."))]);
				return;
			}

			ParameterListRoot->SetContent(Widgets::ListWrapper(
				ColumnSizeData,
				ParametersListScrollbar.ToSharedRef(),
				INVTEXT("Parameters"),
				INVTEXT("External Source"),
				// clang-format off
				SNew(SWidgetSwitcher)
						.WidgetIndex_Lambda([this]() -> int32{ return ParametersList && ParametersList->HasAnyItems() ? 1 : 0; })
						+ SWidgetSwitcher::Slot()
						[
							SNew(STextBlock)
							.Text(INVTEXT("Add parameters to see them here."))
						]
						+ SWidgetSwitcher::Slot()
						[
							ParametersList.ToSharedRef()
						]
				// clang-format on
				));
		}

		void SOUUMaterialAnalyzer::HandleParameterFilterTextChanged(const FText& Text)
		{
			ParameterFilterString = Text.ToString();
			RefreshParamtersList();
		}

		void SOUUMaterialAnalyzer::HandleAssetSelected(const FAssetData& InAssetData)
		{
			auto* EditorObject = GetMutableDefault<UOUUMaterialAnalyzer_EditorObject>();
			SelectedAssetPath = InAssetData.GetSoftObjectPath().ToString();
			if (auto* Material = Cast<UMaterial>(InAssetData.GetAsset()))
			{
				EditorObject->TargetMaterial = Material;
			}
			RefreshParamtersList();
		}

		TSharedRef<SDockTab> SpawnTab(const FSpawnTabArgs& Args)
		{
			// clang-format off
			return SNew(SDockTab)
			.Label(INVTEXT("Material Analyzer (OUU)"))
			[
				SNew(SBox)
				[
					SNew(SOUUMaterialAnalyzer)
				]
			];
			// clang-format on
		}
	} // namespace Widgets

} // namespace OUU::Editor::Private::MaterialAnalyzer

namespace OUU::Editor::MaterialAnalyzer
{
	const FName GTabName = TEXT("OUU.MaterialParametersOverview");

	void RegisterNomadTabSpawner()
	{
		FGlobalTabmanager::Get()
			->RegisterNomadTabSpawner(
				GTabName,
				FOnSpawnTab::CreateStatic(&Private::MaterialAnalyzer::Widgets::SpawnTab))
			.SetDisplayName(INVTEXT("Material Analyzer (OUU)"))
			.SetTooltipText(INVTEXT("Search and navigate through material expressions and parameters"))
			.SetGroup(WorkspaceMenu::GetMenuStructure().GetDeveloperToolsAuditCategory())
			.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "MaterialEditor.ToggleMaterialStats"));
	}

	void UnregisterNomadTabSpawner() { FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(GTabName); }

} // namespace OUU::Editor::MaterialAnalyzer
