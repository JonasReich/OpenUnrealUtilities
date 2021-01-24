// Copyright (c) 2021 Jonas Reich

#include "OUUTests.h"

#if WITH_AUTOMATION_WORKER

#include "UMG/LayerStackWidget.h"
#include "LayerStackWidgetTests.h"

TArray<const UOUULayerWidget*> UTestUOUULayerStackWidget::UpdatedLayers;
TArray<const UOUULayerWidget*> UTestUOUULayerStackWidget::LayersAboveUpdatedLayers;

// This utility function is required, because calling tick directly on the UUserWidget triggers an ensure
void TickLayerStackViaSlate(UOUULayerStackWidget* LayerStack)
{
	auto SlateWidget = LayerStack->TakeWidget();
	FGeometry Geo;
	SlateWidget->Tick(Geo, 0.f, 0.f);
}

BEGIN_DEFINE_SPEC(FLayerStackWidgetSpec, "OpenUnrealUtilities.UMG.LayerStackWidget", DEFAULT_OUU_TEST_FLAGS)
FAutomationTestWorld TestWorld;
UTestUOUULayerStackWidget* StackWidget;
UTestUOUULayerStackWidget* SecondStackWidget;
UTestUOUULayerStackWidget* ThirdStackWidget;
END_DEFINE_SPEC(FLayerStackWidgetSpec)
void FLayerStackWidgetSpec::Define()
{
	BeforeEach([this]()
	{
		TestWorld.CreateWorld();
		TestWorld.InitializeGame();
	});

	Describe("NativeOnInitialized", [this]()
	{
		It("checks if the widget stack is bound and throws an error if the widget stack is not set", [this]()
		{
			AddExpectedError("WidgetStack overlay widget is not bound");
			CreateWidget<UOUULayerStackWidget>(TestWorld.PlayerController);
		});

		It("checks if there are any widgets in the stack overlay and throws a warning if it doesn't have any layer children", [this]()
		{
			AddExpectedError("does not contain any UOUULayerWidget children");
			CreateWidget<UTestUOUULayerStackWidget_Empty>(TestWorld.PlayerController);
		});

		It("doesn't throw any error if the widget stack layer widget is already set and filled with at least one layer", [this]()
		{
			CreateWidget<UTestUOUULayerStackWidget>(TestWorld.PlayerController);
		});
	});

	Describe("", [this]()
	{
		BeforeEach([this]()
		{
			StackWidget = CreateWidget<UTestUOUULayerStackWidget>(TestWorld.PlayerController);
			SecondStackWidget = CreateWidget<UTestUOUULayerStackWidget>(TestWorld.PlayerController);
			ThirdStackWidget = CreateWidget<UTestUOUULayerStackWidget>(TestWorld.PlayerController);
		});

		It("IsLinkedStackHead should throw an error and return false on a stack that was not added to a linked stack", [this]()
		{
			AddExpectedError("not part of a linked stack");
			StackWidget->IsLinkedStackHead();
			// we don't want to test the result because it's guaranteed to be something irrelevant anyways
			// in the current implementation it's true
		});

		It("GetLinkedStackHead should throw an error on a stack that was not added to a linked stack", [this]()
		{
			AddExpectedError("not part of a linked stack");
			StackWidget->GetLinkedStackHead();
			// we don't want to test the result because it's guaranteed to be something irrelevant anyways
			// in the current implementation it's the stack itself
		});

		Describe("", [this]()
		{
			BeforeEach([this]()
			{
				StackWidget->StartNewLinkedStack();
			});

			It("StartNewLinkedStack should set the stack itself as LinkedStackHead", [this]()
			{
				SPEC_TEST_EQUAL(StackWidget->GetLinkedStackHead(), Cast<UOUULayerStackWidget>(StackWidget));
				SPEC_TEST_TRUE(StackWidget->IsLinkedStackHead());
			});

			Describe("InsertToLinkedStackAbove", [this]()
			{
				It("should throw an error when called with invalid param", [this]()
				{
					AddExpectedError("Cannot insert");
					StackWidget->InsertToLinkedStackAbove(nullptr);
				});

				It("should set the newly added stack as linked stack head if added to a single other stack", [this]()
				{
					SecondStackWidget->InsertToLinkedStackAbove(StackWidget);
					SPEC_TEST_TRUE(SecondStackWidget->IsLinkedStackHead());
					SPEC_TEST_FALSE(StackWidget->IsLinkedStackHead());
					SPEC_TEST_EQUAL(StackWidget->GetLinkedStackHead(), Cast<UOUULayerStackWidget>(SecondStackWidget));
					SPEC_TEST_EQUAL(SecondStackWidget->GetStackBelow(), Cast<UOUULayerStackWidget>(StackWidget));
				});

				Describe("to a linked stack that already has two stack elements", [this]()
				{
					BeforeEach([this]()
					{
						SecondStackWidget->InsertToLinkedStackAbove(StackWidget);
					});

					It("should the newly added stack as linked stack head if it's added above the old head", [this]()
					{
						ThirdStackWidget->InsertToLinkedStackAbove(SecondStackWidget);
						SPEC_TEST_TRUE(ThirdStackWidget->IsLinkedStackHead());
						SPEC_TEST_FALSE(SecondStackWidget->IsLinkedStackHead());
						SPEC_TEST_EQUAL(StackWidget->GetLinkedStackHead(), Cast<UOUULayerStackWidget>(ThirdStackWidget));
					});

					It("should not be the head and the head should remain the same if it's added above the root", [this]()
					{
						ThirdStackWidget->InsertToLinkedStackAbove(StackWidget);
						SPEC_TEST_TRUE(SecondStackWidget->IsLinkedStackHead());
						SPEC_TEST_FALSE(ThirdStackWidget->IsLinkedStackHead());
						SPEC_TEST_EQUAL(StackWidget->GetLinkedStackHead(), Cast<UOUULayerStackWidget>(SecondStackWidget));
						SPEC_TEST_EQUAL(ThirdStackWidget->GetStackBelow(), Cast<UOUULayerStackWidget>(StackWidget));
						SPEC_TEST_EQUAL(SecondStackWidget->GetStackBelow(), Cast<UOUULayerStackWidget>(ThirdStackWidget));
					});
				});
			});

			Describe("InsertToLinkedStackBelow", [this]()
			{
				It("should throw an error when called with invalid param", [this]()
				{
					AddExpectedError("Cannot insert");
					StackWidget->InsertToLinkedStackBelow(nullptr);
				});

				It("should leave the existing linked stack head untouched if added to a single other stack", [this]()
				{
					SecondStackWidget->InsertToLinkedStackBelow(StackWidget);
					SPEC_TEST_TRUE(StackWidget->IsLinkedStackHead());
					SPEC_TEST_FALSE(SecondStackWidget->IsLinkedStackHead());
					SPEC_TEST_EQUAL(StackWidget->GetLinkedStackHead(), Cast<UOUULayerStackWidget>(StackWidget));
				});

				It("should still be able to point to the correct stack head when followed up by InsertToLinkedStackAbove", [this]()
				{
					SecondStackWidget->InsertToLinkedStackBelow(StackWidget);
					ThirdStackWidget->InsertToLinkedStackAbove(StackWidget);
					SPEC_TEST_TRUE(ThirdStackWidget->IsLinkedStackHead());
					SPEC_TEST_EQUAL(SecondStackWidget->GetLinkedStackHead(), Cast<UOUULayerStackWidget>(ThirdStackWidget));
				});
			});

			Describe("RemoveFromLinkedStack", [this]()
			{
				It("should remove the stack from the viewport but do nothing else when called on a single stack that is not linked", [this]()
				{
					StackWidget->RemoveFromLinkedStack();
					// no simple way to simulate/verify that it was removed from the viewport, so we just run this to check for errors thrown
				});

				Describe("", [this]()
				{
					BeforeEach([this]()
					{
						SecondStackWidget->InsertToLinkedStackBelow(StackWidget);
						ThirdStackWidget->InsertToLinkedStackBelow(SecondStackWidget);
					});

					It("should remove a linked stack head and turn the stack below into the new stack head", [this]()
					{
						StackWidget->RemoveFromLinkedStack();
						SPEC_TEST_TRUE(SecondStackWidget->IsLinkedStackHead());
						SPEC_TEST_EQUAL(ThirdStackWidget->GetLinkedStackHead(), Cast<UOUULayerStackWidget>(SecondStackWidget));
						SPEC_TEST_NULL(SecondStackWidget->GetStackAbove());
					});

					It("should remove stack from the middle of a linked stack chain without invalidating the stack head", [this]()
					{
						SecondStackWidget->RemoveFromLinkedStack();
						SPEC_TEST_TRUE(StackWidget->IsLinkedStackHead());
						SPEC_TEST_EQUAL(ThirdStackWidget->GetLinkedStackHead(), Cast<UOUULayerStackWidget>(StackWidget));
						SPEC_TEST_EQUAL(StackWidget->GetStackBelow(), Cast<UOUULayerStackWidget>(ThirdStackWidget));
					});

					It("should successfully remove stack from bottom of a linked stack chain", [this]()
					{
						ThirdStackWidget->RemoveFromLinkedStack();
						SPEC_TEST_TRUE(StackWidget->IsLinkedStackHead());
						SPEC_TEST_EQUAL(SecondStackWidget->GetLinkedStackHead(), Cast<UOUULayerStackWidget>(StackWidget));
						SPEC_TEST_NULL(SecondStackWidget->GetStackBelow());
					});
				});
			});

			Describe("NativeTick", [this]()
			{
				It("should call UpdateLayer on all layers inside a stack", [this]()
				{
					TickLayerStackViaSlate(StackWidget);
					
					const TArray<const UOUULayerWidget*> ExpectedUpdatedLayers = { StackWidget->TopLayer, StackWidget->MiddleLayer, StackWidget->BottomLayer };
					SPEC_TEST_ARRAYS_EQUAL(UTestUOUULayerStackWidget::UpdatedLayers, ExpectedUpdatedLayers);
					const TArray<const UOUULayerWidget*> ExpectedLayersAboveUpdatedLayers = { nullptr, StackWidget->TopLayer, StackWidget->MiddleLayer };
					SPEC_TEST_ARRAYS_EQUAL(UTestUOUULayerStackWidget::LayersAboveUpdatedLayers, ExpectedLayersAboveUpdatedLayers);
				});

				It("should not directly update layers on a stack that is not the linked stack head", [this]()
				{
					SecondStackWidget->InsertToLinkedStackBelow(StackWidget);
					TickLayerStackViaSlate(SecondStackWidget);

					const TArray<const UOUULayerWidget*> ExpectedUpdatedLayers = { };
					SPEC_TEST_ARRAYS_EQUAL(UTestUOUULayerStackWidget::UpdatedLayers, ExpectedUpdatedLayers);
					const TArray<const UOUULayerWidget*> ExpectedLayersAboveUpdatedLayers = { };
					SPEC_TEST_ARRAYS_EQUAL(UTestUOUULayerStackWidget::LayersAboveUpdatedLayers, ExpectedLayersAboveUpdatedLayers);
				});

				It("should call UpdateLayer on layers across linked widget stacks", [this]()
				{
					SecondStackWidget->InsertToLinkedStackBelow(StackWidget);
					TickLayerStackViaSlate(StackWidget);

					const TArray<const UOUULayerWidget*> ExpectedUpdatedLayers =
					{
						StackWidget->TopLayer, StackWidget->MiddleLayer, StackWidget->BottomLayer, 
						SecondStackWidget->TopLayer, SecondStackWidget->MiddleLayer, SecondStackWidget->BottomLayer 
					};
					SPEC_TEST_ARRAYS_EQUAL(UTestUOUULayerStackWidget::UpdatedLayers, ExpectedUpdatedLayers);
					const TArray<const UOUULayerWidget*> ExpectedLayersAboveUpdatedLayers =
					{
						nullptr, StackWidget->TopLayer, StackWidget->MiddleLayer,
						StackWidget->BottomLayer, SecondStackWidget->TopLayer, SecondStackWidget->MiddleLayer
					};
					SPEC_TEST_ARRAYS_EQUAL(UTestUOUULayerStackWidget::LayersAboveUpdatedLayers, ExpectedLayersAboveUpdatedLayers);
				});

				AfterEach([this]()
				{
					UTestUOUULayerStackWidget::ClearUpdatedLayers();
				});
			});
		});
	});

	AfterEach([this]()
	{
		TestWorld.DestroyWorld();
	});
}

#endif
