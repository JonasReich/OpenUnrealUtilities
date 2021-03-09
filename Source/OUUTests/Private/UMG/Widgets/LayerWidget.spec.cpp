// Copyright (c) 2021 Jonas Reich

#include "OUUTestUtilities.h"

#if WITH_AUTOMATION_WORKER

#include "Widgets/LayerWidget.h"
#include "Components/HorizontalBox.h"
#include "Components/Button.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "UMG/UMGUtils.h"

UOUULayerWidget* CreateLayer(FAutomationTestWorld& TestWorld)
{
	UOUULayerWidget* ResultLayer = CreateWidget<UOUULayerWidget>(TestWorld.PlayerController);
	UWidgetTree* WidgetTree = ResultLayer->WidgetTree;
	UHorizontalBox* HorizontalBox = WidgetTree->ConstructWidget<UHorizontalBox>();
	WidgetTree->RootWidget = HorizontalBox;

	return ResultLayer;
}

void AddButtonToLayer(UOUULayerWidget* Layer)
{
	UHorizontalBox* HorizontalBox = Cast<UHorizontalBox>(Layer->WidgetTree->RootWidget);
	UButton* Button = Layer->WidgetTree->ConstructWidget<UButton>();
	HorizontalBox->AddChildToHorizontalBox(Button);
}

void SetVisiblityOfAllChildren(UWidget* Widget, ESlateVisibility Visibility)
{
	UMGUtils::ForEachWidgetAndDescendants<UWidget>(Widget, true, [Visibility](UWidget* W) -> bool {
		W->SetVisibility(Visibility);
		return false;
	});
}

BEGIN_DEFINE_SPEC(FLayerWidgetSpec, "OpenUnrealUtilities.UMG.Widgets.LayerWidget", DEFAULT_OUU_TEST_FLAGS)
FAutomationTestWorld TestWorld;
UOUULayerWidget* FirstLayerWidget;
UOUULayerWidget* SecondLayerWidget;
END_DEFINE_SPEC(FLayerWidgetSpec)
void FLayerWidgetSpec::Define()
{
	BeforeEach([this]()
	{
		TestWorld.CreateWorld();
		TestWorld.InitializeGame();

		FirstLayerWidget = CreateLayer(TestWorld);
		SecondLayerWidget = CreateLayer(TestWorld);
	});

	Describe("UpdateLayer updates input visibility of the widget, so calling IsLayerInputVisible", [this]()
	{
		It("should throw an error if the layer itself is focusable", [this]()
		{
			AddExpectedError("Layer widgets themselves must not be focusable!");
			FirstLayerWidget->bIsFocusable = true;
			auto SlateWidget = FirstLayerWidget->TakeWidget();
			FirstLayerWidget->UpdateLayer(nullptr);
		});

		Describe("if bAllowInput is true", [this]()
		{
			BeforeEach([this]()
			{
				FirstLayerWidget->bAllowInput = true;
			});

			It("should return false if there are no input visible children", [this]()
			{
				auto SlateWidget = FirstLayerWidget->TakeWidget();
				FirstLayerWidget->UpdateLayer(nullptr);
				SPEC_TEST_FALSE(FirstLayerWidget->IsLayerInputVisible());
			});

			It("should return true if there are input visible children", [this]()
			{
				AddButtonToLayer(FirstLayerWidget);
				auto SlateWidget = FirstLayerWidget->TakeWidget();
				FirstLayerWidget->UpdateLayer(nullptr);
				SPEC_TEST_TRUE(FirstLayerWidget->IsLayerInputVisible());
			});
		});

		Describe("if bAllowInput is false", [this]()
		{
			BeforeEach([this]()
			{
				FirstLayerWidget->bAllowInput = false;
			});

			It("should return false even if there are input visible children", [this]()
			{
				AddButtonToLayer(FirstLayerWidget);
				auto SlateWidget = FirstLayerWidget->TakeWidget();
				FirstLayerWidget->UpdateLayer(nullptr);
				SPEC_TEST_FALSE(FirstLayerWidget->IsLayerInputVisible());
			});

			It("should return false if there are no input visible children", [this]()
			{
				auto SlateWidget = FirstLayerWidget->TakeWidget();
				FirstLayerWidget->UpdateLayer(nullptr);
				SPEC_TEST_FALSE(FirstLayerWidget->IsLayerInputVisible());
			});
		});
	});

	Describe("UpdateLayer updates actively concealing state, so calling IsActivelyConcealing", [this]()
	{
		Describe("if bConcealLayersBelow is true", [this]()
		{
			BeforeEach([this]()
			{
				FirstLayerWidget->bConcealLayersBelow = true;
			});

			It("should return true if it doesn't have a layer above and it has visible children", [this]()
			{
				SetVisiblityOfAllChildren(FirstLayerWidget, ESlateVisibility::HitTestInvisible);
				auto SlateWidget = FirstLayerWidget->TakeWidget();
				FirstLayerWidget->UpdateLayer(nullptr);
				SPEC_TEST_TRUE(FirstLayerWidget->IsActivelyConcealing());
			});

			It("should return false if it doesn't have a layer above and it has no visible children", [this]()
			{
				SetVisiblityOfAllChildren(FirstLayerWidget, ESlateVisibility::Collapsed);
				auto SlateWidget = FirstLayerWidget->TakeWidget();
				FirstLayerWidget->UpdateLayer(nullptr);
				SPEC_TEST_FALSE(FirstLayerWidget->IsActivelyConcealing());
			});

			Describe("and there is an actively concealing layer above", [this]()
			{
				BeforeEach([this]()
				{
					SecondLayerWidget->bConcealLayersBelow = true;
				});

				It("should return true if it has an actively concealing layer above, but it cannot be concealed", [this]()
				{
					FirstLayerWidget->bMayBeConcealedFromAbove = false;

					auto SlateWidget = FirstLayerWidget->TakeWidget();
					auto SlateWidget2 = SecondLayerWidget->TakeWidget();
					SecondLayerWidget->UpdateLayer(nullptr);
					FirstLayerWidget->UpdateLayer(SecondLayerWidget);

					SPEC_TEST_TRUE(FirstLayerWidget->IsActivelyConcealing());
				});

				It("should return false if it has an actively concealing layer above, and it can be concealed", [this]()
				{
					FirstLayerWidget->bMayBeConcealedFromAbove = true;

					auto SlateWidget = FirstLayerWidget->TakeWidget();
					auto SlateWidget2 = SecondLayerWidget->TakeWidget();
					SecondLayerWidget->UpdateLayer(nullptr);
					FirstLayerWidget->UpdateLayer(SecondLayerWidget);

					SPEC_TEST_FALSE(FirstLayerWidget->IsActivelyConcealing());
				});
			});
		});

		Describe("if bConcealLayersBelow is false", [this]()
		{
			BeforeEach([this]()
			{
				FirstLayerWidget->bConcealLayersBelow = false;
			});

			It("should return false even if it doesn't have a layer above and it has visible children", [this]()
			{
				SetVisiblityOfAllChildren(FirstLayerWidget, ESlateVisibility::SelfHitTestInvisible);
				auto SlateWidget = FirstLayerWidget->TakeWidget();
				FirstLayerWidget->UpdateLayer(nullptr);
				SPEC_TEST_FALSE(FirstLayerWidget->IsActivelyConcealing());
			});

			It("should return false if it doesn't have a layer above and it has no visible children", [this]()
			{
				SetVisiblityOfAllChildren(FirstLayerWidget, ESlateVisibility::Collapsed);
				auto SlateWidget = FirstLayerWidget->TakeWidget();
				FirstLayerWidget->UpdateLayer(nullptr);
				SPEC_TEST_FALSE(FirstLayerWidget->IsActivelyConcealing());
			});

			Describe("and there is an actively concealing layer above", [this]()
			{
				BeforeEach([this]()
				{
					SecondLayerWidget->bConcealLayersBelow = true;
				});

				It("should return false if it has an actively concealing layer above, but it cannot be concealed", [this]()
				{
					FirstLayerWidget->bMayBeConcealedFromAbove = false;

					auto SlateWidget = FirstLayerWidget->TakeWidget();
					auto SlateWidget2 = SecondLayerWidget->TakeWidget();
					SecondLayerWidget->UpdateLayer(nullptr);
					FirstLayerWidget->UpdateLayer(SecondLayerWidget);

					SPEC_TEST_FALSE(FirstLayerWidget->IsActivelyConcealing());
				});

				It("should return false if it has an actively concealing layer above, and it can be concealed", [this]()
				{
					FirstLayerWidget->bMayBeConcealedFromAbove = true;

					auto SlateWidget = FirstLayerWidget->TakeWidget();
					auto SlateWidget2 = SecondLayerWidget->TakeWidget();
					SecondLayerWidget->UpdateLayer(nullptr);
					FirstLayerWidget->UpdateLayer(SecondLayerWidget);

					SPEC_TEST_FALSE(FirstLayerWidget->IsActivelyConcealing());
				});
			});
		});
	});

	Describe("UpdateLayer updates the visibility of widgets, so calling GetVisibility", [this]()
	{
		It("should return Collapsed if there are no visible child widgets", [this]() 
		{
			SetVisiblityOfAllChildren(FirstLayerWidget, ESlateVisibility::Collapsed);
			auto SlateWidget = FirstLayerWidget->TakeWidget();
			FirstLayerWidget->UpdateLayer(nullptr);
			SPEC_TEST_EQUAL(FirstLayerWidget->GetVisibility(), ESlateVisibility::Collapsed);
		});

		It("should return Collapsed if the layer has visible children but is concealed from a layer above", [this]() 
		{
			SetVisiblityOfAllChildren(FirstLayerWidget, ESlateVisibility::Visible);
			auto SlateWidget1 = FirstLayerWidget->TakeWidget();
			auto SlateWidget2 = SecondLayerWidget->TakeWidget();

			SecondLayerWidget->bConcealLayersBelow = true;
			FirstLayerWidget->bMayBeConcealedFromAbove = true;

			SecondLayerWidget->UpdateLayer(nullptr);
			FirstLayerWidget->UpdateLayer(SecondLayerWidget);
			SPEC_TEST_EQUAL(FirstLayerWidget->GetVisibility(), ESlateVisibility::Collapsed);
		});

		It("should return HitTestInvisible if the layer has visible children but is set not to allow any input", [this]() 
		{
			SetVisiblityOfAllChildren(FirstLayerWidget, ESlateVisibility::Visible);
			FirstLayerWidget->bAllowInput = false;
			auto SlateWidget = FirstLayerWidget->TakeWidget();
			FirstLayerWidget->UpdateLayer(nullptr);
			SPEC_TEST_EQUAL(FirstLayerWidget->GetVisibility(), ESlateVisibility::HitTestInvisible);
		});

		It("should return Visible if the layer has any visible children and is set to allow input", [this]() 
		{
			SetVisiblityOfAllChildren(FirstLayerWidget, ESlateVisibility::Visible);
			FirstLayerWidget->bAllowInput = true;
			auto SlateWidget = FirstLayerWidget->TakeWidget();
			FirstLayerWidget->UpdateLayer(nullptr);
			SPEC_TEST_EQUAL(FirstLayerWidget->GetVisibility(), ESlateVisibility::Visible);
		});
	});

	Describe("IsConcealed", [this]()
	{
		It("returns false if the layer is not concealed", [this]()
		{
			auto SlateWidget = FirstLayerWidget->TakeWidget();
			FirstLayerWidget->UpdateLayer(nullptr);
			SPEC_TEST_FALSE(FirstLayerWidget->IsConcealed());
		});

		It("returns true if the layer has a concealing layer above and it's concealable from above", [this]()
		{
			FirstLayerWidget->bMayBeConcealedFromAbove = true;
			SecondLayerWidget->bConcealLayersBelow = true;
			auto SlateWidget1 = FirstLayerWidget->TakeWidget();
			auto SlateWidget2 = SecondLayerWidget->TakeWidget();
			SecondLayerWidget->UpdateLayer(nullptr);
			FirstLayerWidget->UpdateLayer(SecondLayerWidget);
			SPEC_TEST_TRUE(FirstLayerWidget->IsConcealed());
		});

		It("returns false if the layer has a concealing layer above and it's NOT concealable from above", [this]()
		{
			FirstLayerWidget->bMayBeConcealedFromAbove = false;
			SecondLayerWidget->bConcealLayersBelow = true;
			auto SlateWidget1 = FirstLayerWidget->TakeWidget();
			auto SlateWidget2 = SecondLayerWidget->TakeWidget();
			SecondLayerWidget->UpdateLayer(nullptr);
			FirstLayerWidget->UpdateLayer(SecondLayerWidget);
			SPEC_TEST_FALSE(FirstLayerWidget->IsConcealed());
		});

		It("returns true if the layer has a concealing layer above and it's concealable from above even if the layer immediately above is not concealable itself", [this]()
		{
			UOUULayerWidget* ThirdLayerWidget = CreateLayer(TestWorld);

			ThirdLayerWidget->bConcealLayersBelow = true;
			SecondLayerWidget->bMayBeConcealedFromAbove = false;
			FirstLayerWidget->bMayBeConcealedFromAbove = true;

			auto SlateWidget1 = FirstLayerWidget->TakeWidget();
			auto SlateWidget2 = SecondLayerWidget->TakeWidget();
			auto SlateWidget3 = ThirdLayerWidget->TakeWidget();

			ThirdLayerWidget->UpdateLayer(nullptr);
			SecondLayerWidget->UpdateLayer(ThirdLayerWidget);
			FirstLayerWidget->UpdateLayer(SecondLayerWidget);

			SPEC_TEST_TRUE(ThirdLayerWidget->IsActivelyConcealing());
			SPEC_TEST_FALSE(SecondLayerWidget->IsConcealed());
			SPEC_TEST_TRUE(FirstLayerWidget->IsConcealed());
		});
	});

	AfterEach([this]()
	{
		TestWorld.DestroyWorld();
	});
}

#endif
