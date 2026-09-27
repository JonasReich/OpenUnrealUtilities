// Copyright (c) 2026 Jonas Reich & Contributors

#include "GameEntitlements/SOUUGameEntitlementsGrid.h"

#include "GameEntitlements/OUUGameEntitlementsSettings.h"
#include "GameEntitlements/OUUGameEntitlementsTags.h"
#include "GameplayTags/TypedGameplayTag.h"
#include "GameplayTagsManager.h"
#include "GameplayTagsModule.h"
#include "PropertyHandle.h"
#include "ScopedTransaction.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/STableRow.h"

namespace OUU::Editor::Private::GameEntitlements
{
	using namespace OUU::Runtime::GameEntitlements;

	// Wide enough for version names like "MultiplayerPreview" to wrap onto two lines at most.
	static constexpr float GCheckColumnWidth = 96.f;
	static constexpr float GListMaxHeight = 640.f;
	static const FName GNameColumnId = TEXT("Name");
	static const FLinearColor GImplicitCellColor(0.12f, 0.35f, 0.65f, 0.35f);

	//------------------------------------------------------------------------------------------------------------------
	static const FGameplayTagContainer& GetNativeRootTags()
	{
		static const FGameplayTagContainer RootTags = []() {
			FGameplayTagContainer Result;
			Result.AddTag(FOUUGameEntitlementTags::Module::GetTag());
			Result.AddTag(FOUUGameEntitlementTags::Collection::GetTag());
			Result.AddTag(FOUUGameEntitlementTags::Version::GetTag());
			return Result;
		}();
		return RootTags;
	}

	//------------------------------------------------------------------------------------------------------------------
	// Strips the GameEntitlements.Module/Collection/Version prefixes. Tags under additional roots keep their full name.
	static FString GetShortName(const FGameplayTag& Tag)
	{
		return FTypedGameplayTag_Base::ToShortDisplayString(Tag, GetNativeRootTags());
	}

	//------------------------------------------------------------------------------------------------------------------
	static FText GetTagToolTip(const FGameplayTag& Tag)
	{
		FString Comment;
		TArray<FName> TagSources;
		bool bIsTagExplicit = false;
		bool bIsRestrictedTag = false;
		bool bAllowNonRestrictedChildren = false;
		UGameplayTagsManager::Get().GetTagEditorData(
			Tag.GetTagName(),
			OUT Comment,
			OUT TagSources,
			OUT bIsTagExplicit,
			OUT bIsRestrictedTag,
			OUT bAllowNonRestrictedChildren);

		return Comment.IsEmpty() ? FText::FromString(Tag.ToString())
								 : FText::FromString(FString::Printf(TEXT("%s\n%s"), *Tag.ToString(), *Comment));
	}

	//------------------------------------------------------------------------------------------------------------------
	// All registered tags below the root tags, excluding the root tags themselves, sorted by name.
	static TArray<FGameplayTag> GatherChildTags(const FGameplayTagContainer& RootTags)
	{
		const auto& TagsManager = UGameplayTagsManager::Get();

		TArray<FGameplayTag> Result;
		for (const auto& RootTag : RootTags)
		{
			for (const auto& ChildTag : TagsManager.RequestGameplayTagChildren(RootTag))
			{
				Result.AddUnique(ChildTag);
			}
		}

		Result.Sort(
			[](const FGameplayTag& A, const FGameplayTag& B) { return A.GetTagName().LexicalLess(B.GetTagName()); });
		return Result;
	}

	//------------------------------------------------------------------------------------------------------------------
	class SGameEntitlementsGridRow : public SMultiColumnTableRow<TSharedPtr<FGridRow>>
	{
	public:
		SLATE_BEGIN_ARGS(SGameEntitlementsGridRow)
			{
			}
		SLATE_END_ARGS()

		void Construct(
			const FArguments& InArgs,
			const TSharedRef<STableViewBase>& InOwnerTable,
			const TSharedPtr<FGridRow>& InRow,
			const TSharedRef<SGameEntitlementsGrid>& InGrid)
		{
			Row = InRow;
			WeakGrid = InGrid;
			FSuperRowType::Construct(FSuperRowType::FArguments().Padding(FMargin(0.f, 1.f)), InOwnerTable);
		}

		TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
		{
			if (const auto Grid = WeakGrid.Pin())
			{
				return Grid->MakeCellWidget(Row, ColumnName);
			}
			return SNullWidget::NullWidget;
		}

	private:
		TSharedPtr<FGridRow> Row;
		TWeakPtr<SGameEntitlementsGrid> WeakGrid;
	};

	//------------------------------------------------------------------------------------------------------------------
	SGameEntitlementsGrid::~SGameEntitlementsGrid()
	{
		if (auto* Settings = WeakSettings.Get())
		{
			Settings->OnSettingsChanged.Remove(SettingsChangedHandle);
		}
		SettingsChangedHandle.Reset();

		IGameplayTagsModule::OnGameplayTagTreeChanged.Remove(TagTreeChangedHandle);
		TagTreeChangedHandle.Reset();
	}

	//------------------------------------------------------------------------------------------------------------------
	void SGameEntitlementsGrid::Construct(
		const FArguments& InArgs,
		UOUUGameEntitlementSettings* InSettings,
		const TSharedRef<IPropertyHandle>& InEntitlementsHandle)
	{
		WeakSettings = InSettings;
		EntitlementsHandle = InEntitlementsHandle;

		HeaderRow = SNew(SHeaderRow);

		// clang-format off
		ChildSlot
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 4.f)
			[
				SNew(SSearchBox)
				.HintText(INVTEXT("Search entitlement tags"))
				.OnTextChanged(this, &SGameEntitlementsGrid::HandleSearchTextChanged)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 4.f)
			[
				SNew(STextBlock)
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				.AutoWrapText(true)
				.Text(INVTEXT("Checked: explicitly assigned. Tinted with a link icon: implicitly included through a "
					"collection, UnlockAllDLC or a child tag. Hover a cell to see where an inclusion comes from."))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBox)
				.MaxDesiredHeight(GListMaxHeight)
				[
					SAssignNew(ListView, SListView<TSharedPtr<FGridRow>>)
					.ListItemsSource(&FilteredRows)
					.OnGenerateRow(this, &SGameEntitlementsGrid::HandleGenerateRow)
					.SelectionMode(ESelectionMode::None)
					.HeaderRow(HeaderRow)
				]
			]
		];
		// clang-format on

		SettingsChangedHandle =
			InSettings->OnSettingsChanged.AddSP(this, &SGameEntitlementsGrid::HandleSettingsChanged);
		TagTreeChangedHandle = IGameplayTagsModule::OnGameplayTagTreeChanged
								   .AddSP(this, &SGameEntitlementsGrid::HandleGameplayTagTreeChanged);

		RefreshAll();
	}

	//------------------------------------------------------------------------------------------------------------------
	TSharedRef<SWidget> SGameEntitlementsGrid::MakeCellWidget(const TSharedPtr<FGridRow>& Row, const FName& ColumnId)
	{
		if (ColumnId == GNameColumnId)
		{
			return MakeNameCellWidget(Row);
		}

		const TSharedRef<FGridColumn>* ColumnPtr = ColumnsById.Find(ColumnId);
		if (Row->bIsSectionHeader || ColumnPtr == nullptr)
		{
			return SNullWidget::NullWidget;
		}

		const TSharedRef<FGridColumn> Column = *ColumnPtr;
		if (Column->Kind == EGridColumnKind::Collection && Column->Tag == Row->Tag)
		{
			return SNullWidget::NullWidget;
		}

		// clang-format off
		return SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
			.BorderBackgroundColor_Lambda([this, Row, Column]() {
				return FSlateColor(IsImplicit(*Row, *Column) ? GImplicitCellColor : FLinearColor::Transparent);
			})
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Padding(FMargin(2.f, 1.f))
			.ToolTipText_Lambda([this, Row, Column]() { return GetCellToolTip(*Row, *Column); })
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SCheckBox)
					.IsChecked_Lambda([this, Row, Column]() {
						return IsExplicit(*Row, *Column) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
					})
					.OnCheckStateChanged_Lambda([this, Row, Column](ECheckBoxState) { ToggleExplicit(*Row, *Column); })
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(2.f, 0.f, 0.f, 0.f)
				[
					SNew(SImage)
					.Image(FAppStyle::GetBrush("Icons.Link"))
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					.Visibility_Lambda([this, Row, Column]() {
						return IsImplicit(*Row, *Column) ? EVisibility::Visible : EVisibility::Hidden;
					})
				]
			];
		// clang-format on
	}

	//------------------------------------------------------------------------------------------------------------------
	void SGameEntitlementsGrid::RefreshAll()
	{
		RebuildColumns();
		RebuildRows();
		RebuildHeader();
		ApplyFilter();
		ListView->RebuildList();
	}

	//------------------------------------------------------------------------------------------------------------------
	void SGameEntitlementsGrid::RebuildColumns()
	{
		Columns.Reset();
		ColumnsById.Reset();

		const auto* Settings = WeakSettings.Get();
		if (Settings == nullptr)
		{
			return;
		}

		const auto AddColumn = [this](const TSharedRef<FGridColumn>& NewColumn) {
			NewColumn->Id = FName(TEXT("Column"), Columns.Num() + 1);
			ColumnsById.Add(NewColumn->Id, NewColumn);
			Columns.Add(NewColumn);
		};

		for (const auto& VersionTag : GatherChildTags(FOUUGameEntitlementVersion::GetAllRootTags().Get()))
		{
			const auto NewColumn = MakeShared<FGridColumn>();
			NewColumn->Kind = EGridColumnKind::Version;
			NewColumn->Tag = VersionTag;
			NewColumn->Label = FText::FromString(GetShortName(VersionTag));
			NewColumn->ToolTip = GetTagToolTip(VersionTag);
			if (const auto* ExplicitTags = Settings->GetEntitlementsPerVersion().Find(VersionTag))
			{
				NewColumn->ExplicitTags = *ExplicitTags;
			}
			NewColumn->EffectiveTags =
				ResolveEntitlements(*Settings, NewColumn->ExplicitTags, true, &NewColumn->Sources);
			AddColumn(NewColumn);
		}

		for (const auto& CollectionTag : GatherChildTags(FOUUGameEntitlementCollection::GetAllRootTags().Get()))
		{
			const auto NewColumn = MakeShared<FGridColumn>();
			NewColumn->Kind = EGridColumnKind::Collection;
			NewColumn->Tag = CollectionTag;
			NewColumn->Label = FText::FromString(GetShortName(CollectionTag));
			NewColumn->ToolTip = GetTagToolTip(CollectionTag);
			if (const auto* ExplicitTags = Settings->GetModuleCollections().Find(CollectionTag))
			{
				NewColumn->ExplicitTags = *ExplicitTags;
			}
			NewColumn->EffectiveTags =
				ResolveEntitlements(*Settings, NewColumn->ExplicitTags, false, &NewColumn->Sources);
			NewColumn->bHasCycle = NewColumn->EffectiveTags.HasTagExact(CollectionTag);
			AddColumn(NewColumn);
		}

		for (const auto& Dlc : Settings->SteamDlcs)
		{
			const auto NewColumn = MakeShared<FGridColumn>();
			NewColumn->Kind = EGridColumnKind::SteamDlc;
			NewColumn->SteamDlcAppId = Dlc.AppId;
			NewColumn->Label =
				FText::FromString(Dlc.DisplayName.IsEmpty() ? FString::Printf(TEXT("%d"), Dlc.AppId) : Dlc.DisplayName);
			NewColumn->ToolTip = FText::FromString(FString::Printf(TEXT("Steam DLC AppID %d"), Dlc.AppId));
			if (const auto* ExplicitTags = Settings->GetSteamDlcEntitlements().Find(Dlc.AppId))
			{
				NewColumn->ExplicitTags = *ExplicitTags;
			}
			NewColumn->EffectiveTags =
				ResolveEntitlements(*Settings, NewColumn->ExplicitTags, false, &NewColumn->Sources);
			AddColumn(NewColumn);
		}
	}

	//------------------------------------------------------------------------------------------------------------------
	void SGameEntitlementsGrid::RebuildRows()
	{
		AllRows.Reset();

		const auto AddSection =
			[this](const FText& SectionLabel, const TArray<FGameplayTag>& Tags, bool bIsCollection) {
				if (Tags.IsEmpty())
				{
					return;
				}

				const auto SectionHeader = MakeShared<FGridRow>();
				SectionHeader->bIsSectionHeader = true;
				SectionHeader->Label = SectionLabel;
				AllRows.Add(SectionHeader);

				for (const auto& Tag : Tags)
				{
					const auto NewRow = MakeShared<FGridRow>();
					NewRow->Tag = Tag;
					NewRow->bIsCollection = bIsCollection;
					NewRow->Label = FText::FromString(GetShortName(Tag));
					NewRow->ToolTip = GetTagToolTip(Tag);
					AllRows.Add(NewRow);
				}
			};

		AddSection(
			INVTEXT("Collections"),
			GatherChildTags(FOUUGameEntitlementCollection::GetAllRootTags().Get()),
			true);
		AddSection(INVTEXT("Modules"), GatherChildTags(FOUUGameEntitlementModule::GetAllRootTags().Get()), false);
	}

	//------------------------------------------------------------------------------------------------------------------
	void SGameEntitlementsGrid::RebuildHeader()
	{
		HeaderRow->ClearColumns();
		HeaderRow->AddColumn(SHeaderRow::Column(GNameColumnId).DefaultLabel(INVTEXT("Entitlement")).FillWidth(1.f));

		for (const auto& Column : Columns)
		{
			// clang-format off
			HeaderRow->AddColumn(
				SHeaderRow::Column(Column->Id)
				.ManualWidth(GCheckColumnWidth)
				.HAlignHeader(HAlign_Center)
				.HAlignCell(HAlign_Fill)
				.HeaderContent()
				[
					MakeHeaderWidget(Column)
				]);
			// clang-format on
		}
	}

	//------------------------------------------------------------------------------------------------------------------
	void SGameEntitlementsGrid::ApplyFilter()
	{
		FilteredRows.Reset();

		// Section headers are only kept if at least one of their rows passes the filter.
		TSharedPtr<FGridRow> PendingSectionHeader;
		for (const auto& Row : AllRows)
		{
			if (Row->bIsSectionHeader)
			{
				PendingSectionHeader = Row;
				continue;
			}

			if (FilterString.IsEmpty() || Row->Tag.ToString().Contains(FilterString))
			{
				if (PendingSectionHeader.IsValid())
				{
					FilteredRows.Add(PendingSectionHeader);
					PendingSectionHeader.Reset();
				}
				FilteredRows.Add(Row);
			}
		}
	}

	//------------------------------------------------------------------------------------------------------------------
	void SGameEntitlementsGrid::RequestRefresh()
	{
		// Deferred, because settings change notifications arrive from inside a cell's checkbox callback.
		if (bIsRefreshPending)
		{
			return;
		}

		bIsRefreshPending = true;
		RegisterActiveTimer(
			0.f,
			FWidgetActiveTimerDelegate::CreateSP(this, &SGameEntitlementsGrid::HandleRefreshTimer));
	}

	//------------------------------------------------------------------------------------------------------------------
	EActiveTimerReturnType SGameEntitlementsGrid::HandleRefreshTimer(double InCurrentTime, float InDeltaTime)
	{
		bIsRefreshPending = false;
		RefreshAll();
		return EActiveTimerReturnType::Stop;
	}

	//------------------------------------------------------------------------------------------------------------------
	void SGameEntitlementsGrid::HandleSettingsChanged(FPropertyChangedChainEvent& PropertyChangedEvent)
	{
		RequestRefresh();
	}

	//------------------------------------------------------------------------------------------------------------------
	void SGameEntitlementsGrid::HandleGameplayTagTreeChanged() { RequestRefresh(); }

	//------------------------------------------------------------------------------------------------------------------
	void SGameEntitlementsGrid::HandleSearchTextChanged(const FText& InText)
	{
		FilterString = InText.ToString();
		ApplyFilter();
		ListView->RequestListRefresh();
	}

	//------------------------------------------------------------------------------------------------------------------
	TSharedRef<ITableRow> SGameEntitlementsGrid::HandleGenerateRow(
		TSharedPtr<FGridRow> Row,
		const TSharedRef<STableViewBase>& OwnerTable)
	{
		return SNew(SGameEntitlementsGridRow, OwnerTable, Row, SharedThis(this));
	}

	//------------------------------------------------------------------------------------------------------------------
	TSharedRef<SWidget> SGameEntitlementsGrid::MakeNameCellWidget(const TSharedPtr<FGridRow>& Row) const
	{
		if (Row->bIsSectionHeader)
		{
			// clang-format off
			return SNew(SBox)
				.Padding(FMargin(4.f, 6.f, 0.f, 2.f))
				[
					SNew(STextBlock)
					.Font(FAppStyle::Get().GetFontStyle("PropertyWindow.BoldFont"))
					.Text(Row->Label)
				];
			// clang-format on
		}

		// clang-format off
		return SNew(SBox)
			.Padding(FMargin(4.f, 0.f))
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(Row->Label)
				.ToolTipText(Row->ToolTip)
				.HighlightText_Lambda([this]() { return FText::FromString(FilterString); })
			];
		// clang-format on
	}

	//------------------------------------------------------------------------------------------------------------------
	TSharedRef<SWidget> SGameEntitlementsGrid::MakeHeaderWidget(const TSharedRef<FGridColumn>& Column) const
	{
		FText KindText;
		switch (Column->Kind)
		{
		case EGridColumnKind::Version: KindText = INVTEXT("Version"); break;
		case EGridColumnKind::Collection: KindText = INVTEXT("Collection"); break;
		case EGridColumnKind::SteamDlc: KindText = INVTEXT("DLC"); break;
		}

		// clang-format off
		return SNew(SVerticalBox)
			.ToolTipText(Column->ToolTip)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				.Text(KindText)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				[
					SNew(STextBlock)
					.Justification(ETextJustify::Center)
					.AutoWrapText(true)
					.Text(Column->Label)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(2.f, 0.f, 0.f, 0.f)
				[
					SNew(SImage)
					.Image(FAppStyle::GetBrush("Icons.Warning"))
					.ToolTipText(INVTEXT("This collection contains itself through nested collections."))
					.Visibility(Column->bHasCycle ? EVisibility::Visible : EVisibility::Collapsed)
				]
			];
		// clang-format on
	}

	//------------------------------------------------------------------------------------------------------------------
	bool SGameEntitlementsGrid::IsExplicit(const FGridRow& Row, const FGridColumn& Column) const
	{
		return Column.ExplicitTags.HasTagExact(Row.Tag);
	}

	//------------------------------------------------------------------------------------------------------------------
	bool SGameEntitlementsGrid::IsImplicit(const FGridRow& Row, const FGridColumn& Column) const
	{
		if (const auto* RowSources = Column.Sources.Find(Row.Tag))
		{
			if (RowSources->ContainsByPredicate(
					[](const FEntitlementSource& Source) { return Source.Type != EEntitlementSourceType::Explicit; }))
			{
				return true;
			}
		}

		// IsEntitled also matches parent tags of entitled tags.
		return Column.EffectiveTags.HasTagExact(Row.Tag) == false && Column.EffectiveTags.HasTag(Row.Tag);
	}

	//------------------------------------------------------------------------------------------------------------------
	FText SGameEntitlementsGrid::GetCellToolTip(const FGridRow& Row, const FGridColumn& Column) const
	{
		TArray<FString> Lines;
		Lines.Add(FString::Printf(TEXT("%s in %s"), *Row.Label.ToString(), *Column.Label.ToString()));

		if (IsExplicit(Row, Column))
		{
			Lines.Add(TEXT("Explicitly assigned."));
		}

		if (const auto* RowSources = Column.Sources.Find(Row.Tag))
		{
			for (const auto& Source : *RowSources)
			{
				switch (Source.Type)
				{
				case EEntitlementSourceType::Explicit: break;
				case EEntitlementSourceType::Collection:
					Lines.Add(FString::Printf(TEXT("Included via collection %s."), *GetShortName(Source.Via)));
					break;
				case EEntitlementSourceType::UnlockAllDlc:
					Lines.Add(TEXT("Included via UnlockAllDLC (non-shipping builds only)."));
					break;
				}
			}
		}

		if (Column.EffectiveTags.HasTagExact(Row.Tag) == false && Column.EffectiveTags.HasTag(Row.Tag))
		{
			TArray<FString> ChildNames;
			for (const auto& EffectiveTag : Column.EffectiveTags)
			{
				if (EffectiveTag != Row.Tag && EffectiveTag.MatchesTag(Row.Tag))
				{
					ChildNames.Add(GetShortName(EffectiveTag));
				}
			}
			Lines.Add(FString::Printf(
				TEXT("IsEntitled returns true because of child tags: %s."),
				*FString::Join(ChildNames, TEXT(", "))));
		}

		if (Lines.Num() == 1)
		{
			Lines.Add(TEXT("Not included."));
		}

		return FText::FromString(FString::Join(Lines, TEXT("\n")));
	}

	//------------------------------------------------------------------------------------------------------------------
	void SGameEntitlementsGrid::ToggleExplicit(const FGridRow& Row, const FGridColumn& Column)
	{
		auto* Settings = WeakSettings.Get();
		if (Settings == nullptr || EntitlementsHandle.IsValid() == false)
		{
			return;
		}

		const FScopedTransaction Transaction(INVTEXT("Toggle Entitlement Assignment"));
		EntitlementsHandle->NotifyPreChange();
		Settings->Modify();

		auto* Assignment = Settings->Entitlements.FindByPredicate(
			[&Row](const FOUUGameEntitlementAssignment& Candidate) { return Candidate.Entitlement == Row.Tag; });
		if (Assignment == nullptr)
		{
			Assignment = &Settings->Entitlements.AddDefaulted_GetRef();
			Assignment->Entitlement = Row.Tag;
		}

		// Empty rows are removed again in UOUUGameEntitlementSettings::PostEditChangeChainProperty.
		switch (Column.Kind)
		{
		case EGridColumnKind::Version:
			if (Assignment->Versions.HasTagExact(Column.Tag))
			{
				Assignment->Versions.RemoveTag(Column.Tag);
			}
			else
			{
				Assignment->Versions.AddTag(Column.Tag);
			}
			break;
		case EGridColumnKind::Collection:
			if (Assignment->Collections.HasTagExact(Column.Tag))
			{
				Assignment->Collections.RemoveTag(Column.Tag);
			}
			else
			{
				Assignment->Collections.AddTag(Column.Tag);
			}
			break;
		case EGridColumnKind::SteamDlc:
			if (Assignment->SteamDlcAppIds.Remove(Column.SteamDlcAppId) == 0)
			{
				Assignment->SteamDlcAppIds.Add(Column.SteamDlcAppId);
			}
			break;
		}

		EntitlementsHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
		EntitlementsHandle->NotifyFinishedChangingProperties();
	}
} // namespace OUU::Editor::Private::GameEntitlements
