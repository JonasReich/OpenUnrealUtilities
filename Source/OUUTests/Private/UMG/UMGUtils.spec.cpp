// Copyright (c) 2021 Jonas Reich

#include "OUUTestUtilities.h"

#if WITH_AUTOMATION_WORKER

#include "UMG/UMGUtils.h"
#include "Blueprint/UserWidget.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"

void CreateComplexUserWidget(FAutomationTestWorld& TestWorld, UOUUTestWidget* Widget, UWidgetTree* WidgetTree)
{
	UHorizontalBox* HorizontalBox = WidgetTree->ConstructWidget<UHorizontalBox>();
	WidgetTree->RootWidget = HorizontalBox;
	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>();
	HorizontalBox->AddChildToHorizontalBox(TextBlock);
	UImage* Image = WidgetTree->ConstructWidget<UImage>();
	HorizontalBox->AddChildToHorizontalBox(Image);

	// Nested UUserWidget
	UOUUTestWidget* NestedTestWidget = CreateWidget<UOUUTestWidget>(TestWorld.PlayerController);
	HorizontalBox->AddChildToHorizontalBox(NestedTestWidget);
	
	UHorizontalBox* NestedHorizontalBox = NestedTestWidget->WidgetTree->ConstructWidget<UHorizontalBox>();
	NestedTestWidget->WidgetTree->RootWidget = NestedHorizontalBox;
	UButton* NestedButton = NestedTestWidget->WidgetTree->ConstructWidget<UButton>();
	NestedHorizontalBox->AddChildToHorizontalBox(NestedButton);
}

BEGIN_DEFINE_SPEC(FUMGUtilsSpec, "OpenUnrealUtilities.UMG.Utils", DEFAULT_OUU_TEST_FLAGS)
FAutomationTestWorld TestWorld;
UOUUTestWidget* Widget;
UWidgetTree* WidgetTree;
int32 PredicateCallCount;
END_DEFINE_SPEC(FUMGUtilsSpec)
void FUMGUtilsSpec::Define()
{
	BeforeEach([this]()
	{
		TestWorld.CreateWorld();
		TestWorld.InitializeGame();

		Widget = CreateWidget<UOUUTestWidget>(TestWorld.PlayerController);
		WidgetTree = Widget->WidgetTree;

		PredicateCallCount = 0;
	});

	Describe("ForEachWidget", [this]()
	{
		It("should call the predicate on the topmost user widget and its root widget", [this]()
		{
			UMGUtils::ForEachWidget<UWidget>(Widget, [&](UWidget* Widget) -> bool
			{
				PredicateCallCount++;
				return false;
			});
			SPEC_TEST_EQUAL(PredicateCallCount, 2);
		});

		Describe("called on a UserWidget with child widgets added to the widget tree", [this]()
		{
			It("should call the predicate once for every widget in the tree if the predicate always returns false", [this]()
			{
				CreateComplexUserWidget(TestWorld, Widget, WidgetTree);
				UMGUtils::ForEachWidget<UWidget>(Widget, [&](UWidget* Widget) -> bool
				{
					PredicateCallCount++;
					return false;
				});
				SPEC_TEST_EQUAL(PredicateCallCount, 5);
			});

			It("should stop iterating the widgets as soon as one predicate returns true", [this]()
			{
				CreateComplexUserWidget(TestWorld, Widget, WidgetTree);
				UMGUtils::ForEachWidget<UWidget>(Widget, [&](UWidget* Widget) -> bool
				{
					PredicateCallCount++;
					return PredicateCallCount >= 2;
				});
				SPEC_TEST_EQUAL(PredicateCallCount, 2);
			});
		});
	});

	Describe("ForEachWidgetAndDescendants", [this]()
	{
		It("should call the predicate on all widgets in the tree including nested user widgets excluding the root widget if bIncludingRootWidget is false", [this]()
		{
			CreateComplexUserWidget(TestWorld, Widget, WidgetTree);
			UMGUtils::ForEachWidgetAndDescendants<UWidget>(Widget, false, [&](UWidget* Widget) -> bool
			{
				PredicateCallCount++;
				return false;
			});
			SPEC_TEST_EQUAL(PredicateCallCount, 6);
		});

		It("should call the predicate on all widgets in the tree including nested user widgets", [this]()
		{
			CreateComplexUserWidget(TestWorld, Widget, WidgetTree);
			UMGUtils::ForEachWidgetAndDescendants<UWidget>(Widget, true, [&](UWidget* Widget) -> bool
			{
				PredicateCallCount++;
				return false;
			});
			SPEC_TEST_EQUAL(PredicateCallCount, 7);
		});

		It("should stop iterating the widgets as soon as one predicate returns true", [this]()
		{
			CreateComplexUserWidget(TestWorld, Widget, WidgetTree);
			UMGUtils::ForEachWidgetAndDescendants<UWidget>(Widget, true, [&](UWidget* Widget) -> bool
			{
				PredicateCallCount++;
				return PredicateCallCount >= 4;
			});
			SPEC_TEST_EQUAL(PredicateCallCount, 4);
		});
	});

	Describe("ForChildWidgets", [this]()
	{
		It("should not call the predicate on a user widget that does not have any named slots", [this]()
		{
			CreateComplexUserWidget(TestWorld, Widget, WidgetTree);
			UMGUtils::ForChildWidgets<UWidget>(Widget, [&](UWidget* Widget) -> bool
			{
				PredicateCallCount++;
				return false;
			});
			SPEC_TEST_EQUAL(PredicateCallCount, 0);
		});
	});

	Describe("IsFocusable", [this]()
	{
		It("should return false for UHorizontalBox as it can never have focus itself", [this]()
		{
			UHorizontalBox* HorizontalBox = NewObject<UHorizontalBox>();
			auto SlateWidget = HorizontalBox->TakeWidget();
			SPEC_TEST_FALSE(UMGUtils::IsFocusable(HorizontalBox));
		});

		Describe("called on a UButton", [this]()
		{
			It("should return false if the button is marked as NOT focusable", [this]()
			{
				UButton* Button = NewObject<UButton>();
				Button->IsFocusable = false;
				auto SlateWidget = Button->TakeWidget();
				SPEC_TEST_FALSE(UMGUtils::IsFocusable(Button));
			});

			It("should return true if the button is marked as focusable", [this]()
			{
				UButton* Button = NewObject<UButton>();
				Button->IsFocusable = true;
				auto SlateWidget = Button->TakeWidget();
				SPEC_TEST_TRUE(UMGUtils::IsFocusable(Button));
			});
		});

		Describe("called on a UUserWidget", [this]()
		{
			It("should return true if the user widget is marked as focusable", [this]()
			{
				WidgetTree->RootWidget = WidgetTree->ConstructWidget<UButton>();
				Widget->bIsFocusable = true;
				auto SlateWidget = Widget->TakeWidget();
				SPEC_TEST_TRUE(UMGUtils::IsFocusable(Widget));
			});

			It("should return false if the user widget is NOT marked as focusable", [this]()
			{
				WidgetTree->RootWidget = WidgetTree->ConstructWidget<UButton>();
				Widget->bIsFocusable = false;
				auto SlateWidget = Widget->TakeWidget();
				SPEC_TEST_FALSE(UMGUtils::IsFocusable(Widget));
			});
		});
	});

	Describe("IsInputVisible", [this]()
	{
		Describe("called on a button", [this]()
		{
			It("should return true by default", [this]()
			{
				UButton* Button = NewObject<UButton>();
				auto SlateWidget = Button->TakeWidget();
				SPEC_TEST_TRUE(UMGUtils::IsInputVisible(Button));
			});

			It("should return true when disabled but still hit test visible", [this]()
			{
				UButton* Button = NewObject<UButton>();
				Button->bIsEnabled = false;
				Button->SetVisibility(ESlateVisibility::Visible);
				auto SlateWidget = Button->TakeWidget();
				SPEC_TEST_TRUE(UMGUtils::IsInputVisible(Button));
			});

			It("should return true when hit test invisible but still enabled", [this]()
			{
				UButton* Button = NewObject<UButton>();
				Button->bIsEnabled = true;
				Button->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
				auto SlateWidget = Button->TakeWidget();
				SPEC_TEST_TRUE(UMGUtils::IsInputVisible(Button));
			});

			It("should return false if the button is disabled and SelfHitTestVisible", [this]()
			{
				UButton* Button = NewObject<UButton>();
				Button->bIsEnabled = false;
				Button->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
				auto SlateWidget = Button->TakeWidget();
				SPEC_TEST_FALSE(UMGUtils::IsInputVisible(Button));
			});
		});

		Describe("called on a UUserWidget", [this]()
		{
			It("should return false by default", [this]()
			{
				auto SlateWidget = Widget->TakeWidget();
				SPEC_TEST_FALSE(UMGUtils::IsInputVisible(Widget));
			});

			It("should return true when the widget is set to be focusable itself", [this]()
			{
				Widget->bIsFocusable = true;
				auto SlateWidget = Widget->TakeWidget();
				SPEC_TEST_TRUE(UMGUtils::IsInputVisible(Widget));
			});

			It("should return false when the widget has a nested UserWidget that has a focusable/clickable button", [this]()
			{
				CreateComplexUserWidget(TestWorld, Widget, WidgetTree);
				auto SlateWidget = Widget->TakeWidget();
				SPEC_TEST_FALSE(UMGUtils::IsInputVisible(Widget));
			});
		});
	});

	Describe("HasInputVisibleDescendantsExcludingSelf", [this]()
	{
		It("should return false by default when called on a button", [this]()
		{
			UButton* Button = NewObject<UButton>();
			auto SlateWidget = Button->TakeWidget();
			SPEC_TEST_FALSE(UMGUtils::HasInputVisibleDescendantsExcludingSelf(Button));
		});

		Describe("called on a UUserWidget", [this]()
		{
			It("should return false by default", [this]()
			{
				auto SlateWidget = Widget->TakeWidget();
				SPEC_TEST_FALSE(UMGUtils::HasInputVisibleDescendantsExcludingSelf(Widget));
			});

			It("should return false when the widget is set to be focusable itself", [this]()
			{
				Widget->bIsFocusable = true;
				auto SlateWidget = Widget->TakeWidget();
				SPEC_TEST_FALSE(UMGUtils::HasInputVisibleDescendantsExcludingSelf(Widget));
			});

			It("should return true when the widget has a nested UserWidget that has a focusable/clickable button", [this]()
			{
				CreateComplexUserWidget(TestWorld, Widget, WidgetTree);
				auto SlateWidget = Widget->TakeWidget();
				SPEC_TEST_TRUE(UMGUtils::HasInputVisibleDescendantsExcludingSelf(Widget));
			});
		});
	});

	Describe("HasInputVisibleDescendantsIncludingSelf", [this]()
	{
		Describe("called on a button", [this]()
		{
			It("should return true by default", [this]()
			{
				UButton* Button = NewObject<UButton>();
				auto SlateWidget = Button->TakeWidget();
				SPEC_TEST_TRUE(UMGUtils::HasInputVisibleDescendantsIncludingSelf(Button));
			});

			It("should return true when disabled but still hit test visible", [this]()
			{
				UButton* Button = NewObject<UButton>();
				Button->bIsEnabled = false;
				Button->SetVisibility(ESlateVisibility::Visible);
				auto SlateWidget = Button->TakeWidget();
				SPEC_TEST_TRUE(UMGUtils::HasInputVisibleDescendantsIncludingSelf(Button));
			});

			It("should return true when hit test invisible but still enabled", [this]()
			{
				UButton* Button = NewObject<UButton>();
				Button->bIsEnabled = true;
				Button->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
				auto SlateWidget = Button->TakeWidget();
				SPEC_TEST_TRUE(UMGUtils::HasInputVisibleDescendantsIncludingSelf(Button));
			});

			It("should return false if the button is disabled and SelfHitTestVisible", [this]()
			{
				UButton* Button = NewObject<UButton>();
				Button->bIsEnabled = false;
				Button->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
				auto SlateWidget = Button->TakeWidget();
				SPEC_TEST_FALSE(UMGUtils::HasInputVisibleDescendantsIncludingSelf(Button));
			});
		});

		Describe("called on a UUserWidget", [this]()
		{
			It("should return false by default", [this]()
			{
				auto SlateWidget = Widget->TakeWidget();
				SPEC_TEST_FALSE(UMGUtils::HasInputVisibleDescendantsIncludingSelf(Widget));
			});

			It("should return true when the widget is set to be focusable itself", [this]()
			{
				Widget->bIsFocusable = true;
				auto SlateWidget = Widget->TakeWidget();
				SPEC_TEST_TRUE(UMGUtils::HasInputVisibleDescendantsIncludingSelf(Widget));
			});

			It("should return true when the widget has a nested UserWidget that has a focusable/clickable button", [this]()
			{
				CreateComplexUserWidget(TestWorld, Widget, WidgetTree);
				auto SlateWidget = Widget->TakeWidget();
				SPEC_TEST_TRUE(UMGUtils::HasInputVisibleDescendantsIncludingSelf(Widget));
			});
		});
	});

	Describe("GetFirstFocusableDescendantIncludingSelf", [this]()
	{
		It("should return the first button in a horizontal list", [this]()
		{
			UHorizontalBox* HorizontalBox = WidgetTree->ConstructWidget<UHorizontalBox>();
			WidgetTree->RootWidget = HorizontalBox;
			UButton* FirstButton = WidgetTree->ConstructWidget<UButton>();
			HorizontalBox->AddChildToHorizontalBox(FirstButton);
			for (int32 i = 0; i < 4; i++)
			{
				HorizontalBox->AddChildToHorizontalBox(WidgetTree->ConstructWidget<UButton>());
			}
			auto SlateWidget = Widget->TakeWidget();
			UWidget* FirstFocusableDescendant = UMGUtils::GetFirstFocusableDescendantIncludingSelf(Widget);
			SPEC_TEST_TRUE(FirstFocusableDescendant == FirstButton);
		});

		It("should return the UUSerWidget itself if it's focusable itself", [this]()
		{
			WidgetTree->RootWidget = WidgetTree->ConstructWidget<UButton>();
			Widget->bIsFocusable = true;
			auto SlateWidget = Widget->TakeWidget();
			UWidget* FirstFocusableDescendant = UMGUtils::GetFirstFocusableDescendantIncludingSelf(Widget);
			SPEC_TEST_TRUE(FirstFocusableDescendant == Widget);
		});

		It("should return nullptr if there are no focusable descendants", [this]()
		{
			auto SlateWidget = Widget->TakeWidget();
			UWidget* FirstFocusableDescendant = UMGUtils::GetFirstFocusableDescendantIncludingSelf(Widget);
			SPEC_TEST_NULL(FirstFocusableDescendant);
		});
	});

	AfterEach([this]()
	{
		TestWorld.DestroyWorld();
	});
}

#endif
