// Copyright (c) 2021 Jonas Reich

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TextureRenderTargetLibrary.generated.h"

class UTextureRenderTarget2D;

/**
 * 
 */
UCLASS()
class OUURUNTIME_API UTextureRenderTargetLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	/**
	 * Compute the average color of a TextureRenderTarget.
	 * Warning: Very slow (but still a lot faster than doing it in BP)!
	 */
	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Texture Render Target")
	static FLinearColor GetAverageColor(UObject* WorldContextObject, UTextureRenderTarget2D* TextureRenderTarget);

private:
	static EPixelFormat ReadRenderTargetHelper(TArray<FColor>& OutLDRValues, TArray<FLinearColor>& OutHDRValues, UObject* WorldContextObject, UTextureRenderTarget2D* TextureRenderTarget, int32 X, int32 Y, int32 Width, int32 Height);
};
