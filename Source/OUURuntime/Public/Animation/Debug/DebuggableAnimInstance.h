// Copyright (c) 2022 Jonas Reich

#pragma once

#include "CoreMinimal.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"

#include "DebuggableAnimInstance.generated.h"

#if WITH_GAMEPLAY_DEBUGGER
class FGameplayDebuggerCanvasContext;
#endif

UCLASS(abstract)
class OUURUNTIME_API UOUUDebuggableAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
	friend class FGameplayDebuggerCategory_Animation;

#if WITH_GAMEPLAY_DEBUGGER
	virtual void AddGameplayDebuggerInfo(FGameplayDebuggerCanvasContext& CanvasContext) const {}
#endif
};

USTRUCT()
struct OUURUNTIME_API FOUUDebuggableAnimInstanceProxy : public FAnimInstanceProxy
{
	GENERATED_BODY()
public:
	friend class FGameplayDebuggerCategory_Animation;

	FOUUDebuggableAnimInstanceProxy() = default;
	FOUUDebuggableAnimInstanceProxy(UAnimInstance* Instance) : Super(Instance) {}
};
