// Copyright (c) 2022 Jonas Reich

#pragma once

#include "CoreMinimal.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"

#include "DebuggableAnimInstance.generated.h"

UCLASS(abstract)
class OUURUNTIME_API UOUUDebuggableAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
	friend class FGameplayDebuggerCategory_Animation;
};

USTRUCT()
struct OUURUNTIME_API FOUUDebuggableAnimInstanceProxy : public FAnimInstanceProxy
{
	GENERATED_BODY()
public:
	using Super = FAnimInstanceProxy;
	friend class FGameplayDebuggerCategory_Animation;

	FOUUDebuggableAnimInstanceProxy() = default;
	FOUUDebuggableAnimInstanceProxy(UAnimInstance* Instance) : Super(Instance) {}
};
