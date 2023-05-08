// Copyright by Grimlore Games & THQ Nordic

#pragma once

#include "CoreMinimal.h"

#include "EditorSubsystem.h"

#include "OUUEditorSubsystem.generated.h"

UCLASS()
class OUUEDITOR_API UOUUEditorSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	// - USubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	// --
};
