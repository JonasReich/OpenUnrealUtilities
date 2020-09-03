// Copyright (c) 2020 Jonas Reich

#include "UMG/UMGInputBindingTests.h"
#include "OUUTests.h"

#if WITH_AUTOMATION_WORKER

#include "UMG/UMGInputBinding.h"
#include "Engine/World.h"
#include "OpenUnrealUtilitiesMacros.h"
#include "OnlineSubsystemTypes.h"
#include "GameFramework/GameModeBase.h"
#include <GameFramework/PlayerState.h>

#define OUU_TEST_CATEGORY OpenUnrealUtilities
#define OUU_TEST_TYPE UMGInputActionBinding

//////////////////////////////////////////////////////////////////////////

#define ASDF EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter
OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(BindAction, DEFAULT_OUU_TEST_FLAGS)
{
	// Arrange
	/*
	FScopedAutomationWorld TestWorld;
	TestWorld.BeginPlay();
	*/
	static_assert((ASDF & EAutomationTestFlags::ApplicationContextMask) == EAutomationTestFlags::ClientContext, "dum dum dum");
	check(GEngine->GetWorldContexts().Num() == 1);
	check(GEngine->GetWorldContexts()[0].WorldType == EWorldType::Game);

	UWorld* World = GEngine->GetWorldContexts()[0].World();
	FString ErrorString;
	UGameInstance* GI = NewObject<UGameInstance>(GEngine);
	World->SetGameInstance(GI);
	FURL URL(nullptr, TEXT(""), TRAVEL_Absolute);
	World->SetGameMode(URL);
	World->GetAuthGameMode()->InitGame("", "", ErrorString);
	World->GetAuthGameMode()->PlayerStateClass = APlayerState::StaticClass();
	ULocalPlayer* Player = World->GetGameInstance()->CreateLocalPlayer(0, ErrorString, false);
	APlayerController* PlayerController = World->SpawnPlayActor(Player, ENetRole::ROLE_Authority, URL, MakeShared<FUniqueNetIdString>(), ErrorString);
	if (!PlayerController)
		return false;


	UUMGInputBindingTestWidget* Widget = NewObject<UUMGInputBindingTestWidget>(World);
	//Widget->SetPlayerContext(PlayerController);
	UUMGInputActionBindingStack* Stack = UUMGInputActionBindingStack::CreateUMGInputActionBindingStack(Widget);

	FUMGInputAction Action;
	Stack->BindAction(Action, CreateDynamic(FUMGInputActionDelegate, Widget, &UUMGInputBindingTestWidget::HandleInputActionEvent));
	
	// Only the FKey should matter
	FKeyEvent KeyEvent{ EKeys::SpaceBar, FModifierKeysState(), 0, false, 0, 0 };
	
	// Act
	FEventReply Reply = Stack->ProcessOnKeyDown(FGeometry(), KeyEvent);

	// Assert
	TestTrue("Event was handled", Reply.NativeReply.IsEventHandled());

	return true;
}

//////////////////////////////////////////////////////////////////////////

#undef OUU_TEST_CATEGORY
#undef OUU_TEST_TYPE

#endif
