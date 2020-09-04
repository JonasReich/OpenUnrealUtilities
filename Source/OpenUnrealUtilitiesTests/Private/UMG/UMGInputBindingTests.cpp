// Copyright (c) 2020 Jonas Reich

#include "UMG/UMGInputBindingTests.h"
#include "OUUTests.h"

#if WITH_AUTOMATION_WORKER

#include "UMG/UMGInputBinding.h"
#include "Engine/World.h"
#include "OpenUnrealUtilitiesMacros.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"

#define OUU_TEST_CATEGORY OpenUnrealUtilities
#define OUU_TEST_TYPE UMGInputActionBinding

//////////////////////////////////////////////////////////////////////////

#define ASDF EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter
OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(BindAction, DEFAULT_OUU_TEST_FLAGS)
{
	// Arrange
	FScopedAutomationWorld TestWorld;
	TestWorld.BeginPlay();
	if (!TestWorld.InitiailizeGame())
		return false;
	UWorld* World = TestWorld.World;
	
	static FName TestActionName = "TestActionName";
	TestWorld.PlayerController->PlayerInput->AddActionMapping(FInputActionKeyMapping(TestActionName, EKeys::SpaceBar));
	
 	UUMGInputBindingTestWidget* Widget = Cast<UUMGInputBindingTestWidget>(UUserWidget::CreateWidgetInstance(*TestWorld.PlayerController, UUMGInputBindingTestWidget::StaticClass(), NAME_None));
	Widget->AddToPlayerScreen();
	UUMGInputActionBindingStack* Stack = UUMGInputActionBindingStack::CreateUMGInputActionBindingStack(Widget);

	FUMGInputAction Action;
	Action.ActionName = TestActionName;
	Action.ReactEvent = EUMGInputActionKeyEvent::KeyDown;
	Action.ConsumeEvent = EUMGInputActionKeyEvent::KeyDown;
	Stack->BindAction(Action, CreateDynamic(FUMGInputActionDelegate, Widget, &UUMGInputBindingTestWidget::HandleInputActionEvent));
	
	// Only the FKey should matter
	FKeyEvent KeyEvent{ EKeys::SpaceBar, FModifierKeysState(), 0, false, 0, 0 };
	
	// Act
	TestWorld.PlayerController->PlayerInput->InputKey(EKeys::SpaceBar, EInputEvent::IE_Pressed, 0.f, false);
	TArray<UInputComponent*> InputComponents;
	TestWorld.PlayerController->PlayerInput->ProcessInputStack(InputComponents, 0.f, false);
	FEventReply Reply = Stack->ProcessOnKeyDown(FGeometry(), KeyEvent);

	// Assert
	TestTrue("Event was handled", Reply.NativeReply.IsEventHandled());

	return true;
}

//////////////////////////////////////////////////////////////////////////

#undef OUU_TEST_CATEGORY
#undef OUU_TEST_TYPE

#endif
