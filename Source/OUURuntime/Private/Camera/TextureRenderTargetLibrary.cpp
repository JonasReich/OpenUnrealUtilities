// Copyright (c) 2021 Jonas Reich

#include "Camera/TextureRenderTargetLibrary.h"
#include "Engine/TextureRenderTarget2D.h"

// Copied from Private\KismetRenderingLibrary.cpp:220

EPixelFormat UTextureRenderTargetLibrary::ReadRenderTargetHelper(
	TArray<FColor>& OutLDRValues,
	TArray<FLinearColor>& OutHDRValues,
	UObject* WorldContextObject,
	UTextureRenderTarget2D* TextureRenderTarget,
	int32 X,
	int32 Y,
	int32 Width,
	int32 Height)
{
	EPixelFormat OutFormat = PF_Unknown;

	if (!TextureRenderTarget)
	{
		return OutFormat;
	}

	FTextureRenderTarget2DResource* RTResource = (FTextureRenderTarget2DResource*)TextureRenderTarget->GameThread_GetRenderTargetResource();
	if (!RTResource)
	{
		return OutFormat;
	}

	X = FMath::Clamp(X, 0, TextureRenderTarget->SizeX - 1);
	Y = FMath::Clamp(Y, 0, TextureRenderTarget->SizeY - 1);
	Width = FMath::Clamp(Width, 1, TextureRenderTarget->SizeX);
	Height = FMath::Clamp(Height, 1, TextureRenderTarget->SizeY);
	Width = Width - FMath::Max(X + Width - TextureRenderTarget->SizeX, 0);
	Height = Height - FMath::Max(Y + Height - TextureRenderTarget->SizeY, 0);

	FIntRect SampleRect(X, Y, X + Width, Y + Height);
	FReadSurfaceDataFlags ReadSurfaceDataFlags;

	FRenderTarget* RenderTarget = TextureRenderTarget->GameThread_GetRenderTargetResource();
	OutFormat = TextureRenderTarget->GetFormat();

	const int32 NumPixelsToRead = Width * Height;

	switch (OutFormat)
	{
	case PF_B8G8R8A8:
		OutLDRValues.SetNumUninitialized(NumPixelsToRead);
		if (!RenderTarget->ReadPixelsPtr(OutLDRValues.GetData(), ReadSurfaceDataFlags, SampleRect))
		{
			OutFormat = PF_Unknown;
		}
		break;
	case PF_FloatRGBA:
		OutHDRValues.SetNumUninitialized(NumPixelsToRead);
		if (!RenderTarget->ReadLinearColorPixelsPtr(OutHDRValues.GetData(), ReadSurfaceDataFlags, SampleRect))
		{
			OutFormat = PF_Unknown;
		}
		break;
	default:
		OutFormat = PF_Unknown;
		break;
	}

	return OutFormat;
}

FLinearColor UTextureRenderTargetLibrary::GetAverageColor(UObject* WorldContextObject, UTextureRenderTarget2D* TextureRenderTarget)
{
	TArray<FColor> Samples;
	TArray<FLinearColor> LinearSamples;
	FLinearColor Average = FLinearColor::Black;

	float TotalPixelCount = TextureRenderTarget->SizeX * TextureRenderTarget->SizeY;
	switch (ReadRenderTargetHelper(Samples, LinearSamples, WorldContextObject, TextureRenderTarget, 0, 0, TextureRenderTarget->SizeX, TextureRenderTarget->SizeY))
	{
	case PF_B8G8R8A8:
		check(Samples.Num() == TotalPixelCount && LinearSamples.Num() == 0);
		for (const FColor& Color : Samples)
		{
			// I don't know why they use this, instead of the constructor conversion (which remaps properly to linear space, instead of directly assigning)
			Average += FLinearColor(float(Color.R), float(Color.G), float(Color.B), float(Color.A));
		}
		Average /= TotalPixelCount;
		break;
	case PF_FloatRGBA:
		check(LinearSamples.Num() == TotalPixelCount && Samples.Num() == 0);
		for (const FLinearColor& Color : LinearSamples)
		{
			Average += Color;
		}
		Average /= TotalPixelCount;
		break;
	case PF_Unknown:
	default:
		break;
	}

	return Average;
}
