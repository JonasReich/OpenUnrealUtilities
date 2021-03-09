// Copyright (c) 2021 Jonas Reich

#include "UMG/UMGInputBindingTests.h"
#include "OUUTestUtilities.h"

#if WITH_AUTOMATION_WORKER

#include "UMGInputBinding.h"
#include "OUUMacros.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"

FKeyEvent SimulateKeyEvent(UPlayerInput* PlayerInput, UUMGInputBindingTestWidget* Widget, FKey Key, EInputEvent Event)
{
	FKeyEvent KeyEvent{ Key, FModifierKeysState(), 0, false, 0, 0 };
	PlayerInput->InputKey(Key, Event, 0.f, false);
	TArray<UInputComponent*> InputComponents;
	PlayerInput->ProcessInputStack(InputComponents, 0.f, false);
	switch (Event)
	{
	case IE_Pressed:
	{
		Widget->NativeOnKeyDown(FGeometry(), KeyEvent);
		break;
	}
	case IE_Released:
	{
		Widget->NativeOnKeyUp(FGeometry(), KeyEvent);
		break;
	}
	}
	return KeyEvent;
}

BEGIN_DEFINE_SPEC(FUMGInputActionBindingSpec, "OpenUnrealUtilities.UMG.InputActionBinding", DEFAULT_OUU_TEST_FLAGS)
FAutomationTestWorld TestWorld;
UPlayerInput* PlayerInput;
UUMGInputBindingTestWidget* Widget;
UUMGInputActionBindingStack* Stack;
END_DEFINE_SPEC(FUMGInputActionBindingSpec)
void FUMGInputActionBindingSpec::Define()
{
	BeforeEach([this]()
	{
		TestWorld.CreateWorld();
		TestWorld.InitializeGame();
		PlayerInput = TestWorld.PlayerController->PlayerInput;

		Widget = Cast<UUMGInputBindingTestWidget>(UUserWidget::CreateWidgetInstance(*TestWorld.PlayerController, UUMGInputBindingTestWidget::StaticClass(), NAME_None));
		Widget->AddToPlayerScreen();
		Stack = UUMGInputActionBindingStack::CreateUMGInputActionBindingStack(Widget);
		Widget->Stack = Stack;
	});

	Describe("CreateUMGInputActionBindingStack", [this]()
	{
		It("should create a valid stack with the correct player input set", [this]()
		{
			TestTrue("valid stack", IsValid(Stack));
			TestEqual("OwningPlayerInput", Stack->GetOwningPlayerInput(), PlayerInput);
		});

		It("should throw an error and return nullptr if the provided widget is invalid", [this]()
		{
			AddExpectedError("Could not create UMGInputActionBindingStack with invalid OwningWidget!", EAutomationExpectedErrorFlags::Exact, 1);
			UUMGInputActionBindingStack* ResultStack = UUMGInputActionBindingStack::CreateUMGInputActionBindingStack(nullptr);
			TestNull("Stack created from nullptr widget", ResultStack);
		});
	});

	Describe("SetOwningPlayerInput", [this]()
	{
		It("should change the owning player input", [this]()
		{
			Stack->SetOwningPlayerInput(nullptr);
			TestNull("OwningPlayer after setting nullptr", Stack->GetOwningPlayerInput());
			Stack->SetOwningPlayerInput(PlayerInput);
			TestTrue("OwningPlayer is same", Stack->GetOwningPlayerInput() == PlayerInput);
		});
	});

	Describe("Binding a simple key down event", [this]()
	{
		BeforeEach([this]() 
		{
			const FName TestActionName = "TestAction";
			PlayerInput->AddActionMapping(FInputActionKeyMapping(TestActionName, EKeys::One));
			FUMGInputAction ActionOne{ TestActionName, EUMGInputActionKeyEvent::KeyDown, EUMGInputActionKeyEventConsumeMode::Same };
			Stack->BindAction(ActionOne, CreateDynamic(FUMGInputActionDelegate, Widget, &UUMGInputBindingTestWidget::HandleInputActionEventOne));
		});
		
		It("should result in a single bindings being present with the target object", [this]()
		{
			TestEqual("Number of bindings", Stack->GetNumBindingsToObject(Widget), 1);
		});

		It("should result in a delegate call and event consumption if the same key is pressed", [this]()
		{
			SimulateKeyEvent(PlayerInput, Widget, EKeys::One, IE_Pressed);
			TestEqual("Number of delegate calls", Widget->HandleInputActionEventOne_NumCalls, 1);
			TestTrue("Input event was consumed", Widget->bLastNativeOnKeyDownWasHandled);
		});
		
		It("should result in NO delegate call and event consumption if the same key is released", [this]()
		{
			SimulateKeyEvent(PlayerInput, Widget, EKeys::One, IE_Released);
			TestEqual("Number of delegate calls", Widget->HandleInputActionEventOne_NumCalls, 0);
			TestFalse("Input event was consumed", Widget->bLastNativeOnKeyDownWasHandled);
		});

		It("should result in two delegate calls if the same key is pressed twice", [this]() 
		{
			SimulateKeyEvent(PlayerInput, Widget, EKeys::One, IE_Pressed);
			SimulateKeyEvent(PlayerInput, Widget, EKeys::One, IE_Pressed);
			TestEqual("Number of delegate calls", Widget->HandleInputActionEventOne_NumCalls, 2);
		});
	});

	Describe("Binding two events with different keys", [this]()
	{
		BeforeEach([this]()
		{
			const FName FirstActionName = "FirstAction";
			PlayerInput->AddActionMapping(FInputActionKeyMapping(FirstActionName, EKeys::One));
			FUMGInputAction ActionOne{ FirstActionName, EUMGInputActionKeyEvent::KeyDown, EUMGInputActionKeyEventConsumeMode::Same };
			Stack->BindAction(ActionOne, CreateDynamic(FUMGInputActionDelegate, Widget, &UUMGInputBindingTestWidget::HandleInputActionEventOne));
			
			const FName SecondActionName = "SecondAction";
			PlayerInput->AddActionMapping(FInputActionKeyMapping(SecondActionName, EKeys::Two));
			FUMGInputAction ActionTwo{ SecondActionName, EUMGInputActionKeyEvent::KeyDown, EUMGInputActionKeyEventConsumeMode::Same };
			Stack->BindAction(ActionTwo, CreateDynamic(FUMGInputActionDelegate, Widget, &UUMGInputBindingTestWidget::HandleInputActionEventTwo));
		});

		It("should result in two bindings being present with the target object", [this]()
		{
			TestEqual("Number of bindings", Stack->GetNumBindingsToObject(Widget), 2);
		});

		It("should trigger the first action if the matching key is pressed", [this]()
		{
			SimulateKeyEvent(PlayerInput, Widget, EKeys::One, IE_Pressed);
			TestEqual("Number of delegate calls (first action)", Widget->HandleInputActionEventOne_NumCalls, 1);
			TestEqual("Number of delegate calls (second action)", Widget->HandleInputActionEventTwo_NumCalls, 0);
		});

		It("should trigger the second action if the matching key is pressed", [this]()
		{
			SimulateKeyEvent(PlayerInput, Widget, EKeys::Two, IE_Pressed);
			TestEqual("Number of delegate calls (first action)", Widget->HandleInputActionEventOne_NumCalls, 0);
			TestEqual("Number of delegate calls (second action)", Widget->HandleInputActionEventTwo_NumCalls, 1);
		});
	});

	Describe("Binding two events with the same key", [this]()
	{
		BeforeEach([this]()
		{
			const FName FirstActionName = "FirstAction";
			PlayerInput->AddActionMapping(FInputActionKeyMapping(FirstActionName, EKeys::One));
			FUMGInputAction ActionOne{ FirstActionName, EUMGInputActionKeyEvent::KeyDown, EUMGInputActionKeyEventConsumeMode::Same };
			Stack->BindAction(ActionOne, CreateDynamic(FUMGInputActionDelegate, Widget, &UUMGInputBindingTestWidget::HandleInputActionEventOne));

			const FName SecondActionName = "SecondAction";
			PlayerInput->AddActionMapping(FInputActionKeyMapping(SecondActionName, EKeys::One));
			FUMGInputAction ActionTwo{ SecondActionName, EUMGInputActionKeyEvent::KeyDown, EUMGInputActionKeyEventConsumeMode::Same };
			Stack->BindAction(ActionTwo, CreateDynamic(FUMGInputActionDelegate, Widget, &UUMGInputBindingTestWidget::HandleInputActionEventTwo));
		});

		It("should trigger the first bound action if the matching key is pressed", [this]()
		{
			SimulateKeyEvent(PlayerInput, Widget, EKeys::One, IE_Pressed);
			TestEqual("Number of delegate calls (first action)", Widget->HandleInputActionEventOne_NumCalls, 1);
			TestEqual("Number of delegate calls (second action)", Widget->HandleInputActionEventTwo_NumCalls, 0);
		});
	});

	Describe("Marking an action as bIsOneshot", [this]()
	{
		BeforeEach([this]()
		{
			const FName FirstActionName = "FirstAction";
			PlayerInput->AddActionMapping(FInputActionKeyMapping(FirstActionName, EKeys::One));
			FUMGInputAction ActionOne{ FirstActionName, EUMGInputActionKeyEvent::KeyDown, EUMGInputActionKeyEventConsumeMode::Same, /*bIsOneshot*/ true };
			Stack->BindAction(ActionOne, CreateDynamic(FUMGInputActionDelegate, Widget, &UUMGInputBindingTestWidget::HandleInputActionEventOne));
		});

		It("should unbind the event after the first call", [this]()
		{
			SimulateKeyEvent(PlayerInput, Widget, EKeys::One, IE_Pressed);
			TestEqual("Number of bindings", Stack->GetNumBindingsToObject(Widget), 0);
			SimulateKeyEvent(PlayerInput, Widget, EKeys::One, IE_Pressed);
			TestEqual("Number of delegate calls", Widget->HandleInputActionEventOne_NumCalls, 1);
		});
	});

	Describe("RemoveSingleBinding", [this]()
	{
		It("should remove only the binding specified", [this]()
		{
			const FName FirstActionName = "FirstAction";
			PlayerInput->AddActionMapping(FInputActionKeyMapping(FirstActionName, EKeys::One));
			FUMGInputAction ActionOne{ FirstActionName, EUMGInputActionKeyEvent::KeyDown, EUMGInputActionKeyEventConsumeMode::Same };
			Stack->BindAction(ActionOne, CreateDynamic(FUMGInputActionDelegate, Widget, &UUMGInputBindingTestWidget::HandleInputActionEventOne));

			const FName SecondActionName = "SecondAction";
			PlayerInput->AddActionMapping(FInputActionKeyMapping(SecondActionName, EKeys::One));
			FUMGInputAction ActionTwo{ SecondActionName, EUMGInputActionKeyEvent::KeyDown, EUMGInputActionKeyEventConsumeMode::Same };
			Stack->BindAction(ActionTwo, CreateDynamic(FUMGInputActionDelegate, Widget, &UUMGInputBindingTestWidget::HandleInputActionEventTwo));

			Stack->RemoveSingleBinding(ActionTwo, CreateDynamic(FUMGInputActionDelegate, Widget, &UUMGInputBindingTestWidget::HandleInputActionEventTwo));

			TestEqual("Number of bindings", Stack->GetNumBindingsToObject(Widget), 1);
		});

		It("should not remove a binding if the delegate doesn't match", [this]()
		{
			const FName FirstActionName = "FirstAction";
			PlayerInput->AddActionMapping(FInputActionKeyMapping(FirstActionName, EKeys::One));
			FUMGInputAction ActionOne{ FirstActionName, EUMGInputActionKeyEvent::KeyDown, EUMGInputActionKeyEventConsumeMode::Same };
			Stack->BindAction(ActionOne, CreateDynamic(FUMGInputActionDelegate, Widget, &UUMGInputBindingTestWidget::HandleInputActionEventOne));
			
			Stack->RemoveSingleBinding(ActionOne, CreateDynamic(FUMGInputActionDelegate, Widget, &UUMGInputBindingTestWidget::HandleInputActionEventTwo));

			TestEqual("Number of bindings", Stack->GetNumBindingsToObject(Widget), 1);
		});
	});
	
	It("RemoveBindings should remove the bindings of the same type", [this]()
	{
		const FName FirstActionName = "FirstAction";
		PlayerInput->AddActionMapping(FInputActionKeyMapping(FirstActionName, EKeys::One));
		FUMGInputAction ActionOne{ FirstActionName, EUMGInputActionKeyEvent::KeyDown, EUMGInputActionKeyEventConsumeMode::Same };
		Stack->BindAction(ActionOne, CreateDynamic(FUMGInputActionDelegate, Widget, &UUMGInputBindingTestWidget::HandleInputActionEventOne));

		const FName SecondActionName = "SecondAction";
		PlayerInput->AddActionMapping(FInputActionKeyMapping(SecondActionName, EKeys::One));
		FUMGInputAction ActionTwo{ SecondActionName, EUMGInputActionKeyEvent::KeyDown, EUMGInputActionKeyEventConsumeMode::Same };
		Stack->BindAction(ActionTwo, CreateDynamic(FUMGInputActionDelegate, Widget, &UUMGInputBindingTestWidget::HandleInputActionEventTwo));
		Stack->BindAction(ActionTwo, CreateDynamic(FUMGInputActionDelegate, Widget, &UUMGInputBindingTestWidget::HandleInputActionEventThree));

		TestEqual("Number of bindings", Stack->GetNumBindingsToObject(Widget), 3);
		Stack->RemoveBindings(ActionTwo);
		TestEqual("Number of bindings", Stack->GetNumBindingsToObject(Widget), 1);
	});

	It("RemoveAllBindings should remove all bindings no matter the signature", [this]()
	{
		const FName FirstActionName = "FirstAction";
		PlayerInput->AddActionMapping(FInputActionKeyMapping(FirstActionName, EKeys::One));
		FUMGInputAction ActionOne{ FirstActionName, EUMGInputActionKeyEvent::KeyDown, EUMGInputActionKeyEventConsumeMode::Same };
		Stack->BindAction(ActionOne, CreateDynamic(FUMGInputActionDelegate, Widget, &UUMGInputBindingTestWidget::HandleInputActionEventOne));

		const FName SecondActionName = "SecondAction";
		PlayerInput->AddActionMapping(FInputActionKeyMapping(SecondActionName, EKeys::One));
		FUMGInputAction ActionTwo{ SecondActionName, EUMGInputActionKeyEvent::KeyDown, EUMGInputActionKeyEventConsumeMode::Same };
		Stack->BindAction(ActionTwo, CreateDynamic(FUMGInputActionDelegate, Widget, &UUMGInputBindingTestWidget::HandleInputActionEventTwo));
		Stack->BindAction(ActionTwo, CreateDynamic(FUMGInputActionDelegate, Widget, &UUMGInputBindingTestWidget::HandleInputActionEventThree));

		TestEqual("Number of bindings", Stack->GetNumBindingsToObject(Widget), 3);
		Stack->RemoveAllBindings();
		TestEqual("Number of bindings", Stack->GetNumBindingsToObject(Widget), 0);
	});

	It("RemoveBindingByObject should remove only the bindings to the target object", [this]()
	{
		UUMGInputBindingTestWidget* SecondWidget = CreateWidget<UUMGInputBindingTestWidget>(TestWorld.PlayerController, UUMGInputBindingTestWidget::StaticClass(), NAME_None);

		const FName FirstActionName = "FirstAction";
		PlayerInput->AddActionMapping(FInputActionKeyMapping(FirstActionName, EKeys::One));
		FUMGInputAction ActionOne{ FirstActionName, EUMGInputActionKeyEvent::KeyDown, EUMGInputActionKeyEventConsumeMode::Same };
		Stack->BindAction(ActionOne, CreateDynamic(FUMGInputActionDelegate, Widget, &UUMGInputBindingTestWidget::HandleInputActionEventOne));
		Stack->BindAction(ActionOne, CreateDynamic(FUMGInputActionDelegate, SecondWidget, &UUMGInputBindingTestWidget::HandleInputActionEventOne));

		const FName SecondActionName = "SecondAction";
		PlayerInput->AddActionMapping(FInputActionKeyMapping(SecondActionName, EKeys::One));
		FUMGInputAction ActionTwo{ SecondActionName, EUMGInputActionKeyEvent::KeyDown, EUMGInputActionKeyEventConsumeMode::Same };
		Stack->BindAction(ActionTwo, CreateDynamic(FUMGInputActionDelegate, Widget, &UUMGInputBindingTestWidget::HandleInputActionEventTwo));
		Stack->BindAction(ActionTwo, CreateDynamic(FUMGInputActionDelegate, SecondWidget, &UUMGInputBindingTestWidget::HandleInputActionEventTwo));

		TestEqual("Number of bindings (widget 1) before removal", Stack->GetNumBindingsToObject(Widget), 2);
		TestEqual("Number of bindings (widget 2) before removal", Stack->GetNumBindingsToObject(SecondWidget), 2);
		Stack->RemoveBindingsByObject(Widget);
		TestEqual("Number of bindings (widget 1) after removal", Stack->GetNumBindingsToObject(Widget), 0);
		TestEqual("Number of bindings (widget 2) after removal", Stack->GetNumBindingsToObject(SecondWidget), 2);
	});

	AfterEach([this]() 
	{
		TestWorld.DestroyWorld();
	});
}

#endif

void UUMGInputBindingTestWidget::HandleInputActionEventOne(EUMGInputActionKeyEvent SourceEvent)
{
	HandleInputActionEventOne_NumCalls++;
	HandleInputActionEventOne_SourceEvent = SourceEvent;
}

void UUMGInputBindingTestWidget::HandleInputActionEventTwo(EUMGInputActionKeyEvent SourceEvent)
{
	HandleInputActionEventTwo_NumCalls++;
	HandleInputActionEventTwo_SourceEvent = SourceEvent;
}

void UUMGInputBindingTestWidget::HandleInputActionEventThree(EUMGInputActionKeyEvent SourceEvent)
{
	// do nothing
}

FReply UUMGInputBindingTestWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (IsValid(Stack))
	{
		auto Result = Stack->ProcessOnKeyDown(InGeometry, InKeyEvent);
		if (Result.NativeReply.IsEventHandled())
		{
			bLastNativeOnKeyDownWasHandled = true;
			return Result.NativeReply;
		}
	}
	bLastNativeOnKeyDownWasHandled = false;
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UUMGInputBindingTestWidget::NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (IsValid(Stack))
	{
		auto Result = Stack->ProcessOnKeyUp(InGeometry, InKeyEvent);
		if (Result.NativeReply.IsEventHandled())
		{
			bLastNativeOnKeyUpWasHandled = true;
			return Result.NativeReply;
		}
	}
	bLastNativeOnKeyUpWasHandled = false;
	return Super::NativeOnKeyUp(InGeometry, InKeyEvent);
}
