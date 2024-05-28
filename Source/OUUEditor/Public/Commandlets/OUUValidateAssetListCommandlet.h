// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "Commandlets/Commandlet.h"

#include "OUUValidateAssetListCommandlet.generated.h"


UCLASS()
class OUUEDITOR_API UOUUValidateAssetListCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	// - UCommandlet
	int32 Main(const FString& FullCommandLine) override;
	// --
};
