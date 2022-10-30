// Copyright by Grimlore Games & THQ Nordic

#include "Commandlets/ExportWorldPartitionMiniMapBuilder.h"

#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include "ImageWriteBlueprintLibrary.h"
#include "ImageWriteQueue.h"
#include "ImageWriteTask.h"
#include "Modules/ModuleManager.h"
#include "Serialization/BufferArchive.h"
#include "WorldPartition/WorldPartitionMiniMapHelper.h"

// Use custom implementation until we find a fix for the following issue:
// The UImageWriteBlueprintLibrary function always saves a default 32px texture,
// because the RHI texture resource is not loaded properly.
#define USE_CUSTOM_EXPORT_IMPL 1

#if USE_CUSTOM_EXPORT_IMPL
namespace OUU::Editor::Private
{
	TArray64<FColor> GetTexturePixels(FTextureSource& TextureSource)
	{
		// see UAsyncImageExport::Activate()

		TArray64<uint8> RawTextureData;
		TextureSource.GetMipData(OUT RawTextureData, 0);

		const int32 Width = TextureSource.GetSizeX();
		const int32 Height = TextureSource.GetSizeY();

		const int32 BytesPerPixel = TextureSource.GetBytesPerPixel();
		const ETextureSourceFormat SourceFormat = TextureSource.GetFormat();
		checkf(SourceFormat == TSF_BGRA8 || SourceFormat == TSF_BGRE8, TEXT("Unsupported format"));

		TArray64<FColor> OutPixels;
		OutPixels.SetNumZeroed(Width * Height);

		for (int32 Y = 0; Y < Height; ++Y)
		{
			for (int32 X = 0; X < Width; ++X)
			{
				const int32 PixelByteOffset = (X + Y * Width) * BytesPerPixel;
				const uint8* PixelPtr = RawTextureData.GetData() + PixelByteOffset;
				auto& Color = OutPixels[Y * Width + X];

				Color = *((FColor*)PixelPtr);
				// Force no alpha
				Color.A = 255;
			}
		}

		return OutPixels;
	}

	bool ExportTexture(UTexture* Texture, const FString ExportPath, FImageWriteOptions InOptions)
	{
		FTextureSource& TextureSource = Texture->Source;

		FIntPoint ImageSize(TextureSource.GetSizeX(), TextureSource.GetSizeY());
		TUniquePtr<FImageWriteTask> ImageTask = MakeUnique<FImageWriteTask>();
		ImageTask->PixelData = MakeUnique<TImagePixelData<FColor>>(ImageSize, GetTexturePixels(TextureSource));
		ImageTask->Format = ImageFormatFromDesired(InOptions.Format);
		ImageTask->OnCompleted = InOptions.NativeOnComplete;
		ImageTask->Filename = ExportPath;
		ImageTask->bOverwriteFile = InOptions.bOverwriteFile;
		ImageTask->CompressionQuality = InOptions.CompressionQuality;

		auto& ImageWriteQueue =
			FModuleManager::Get().LoadModuleChecked<IImageWriteQueueModule>("ImageWriteQueue").GetWriteQueue();
		TFuture<bool> ImageWriteTask = ImageWriteQueue.Enqueue(MoveTemp(ImageTask));

		// Block until task is finished
		check(!InOptions.bAsync);
		ImageWriteTask.Wait();
		return ImageWriteTask.Get();
	}

} // namespace OUU::Editor::Private
#endif

bool UExportWorldPartitionMiniMapBuilder::PreRun(UWorld* World, FPackageSourceControlHelper& PackageHelper)
{
	WorldMiniMap = FWorldPartitionMiniMapHelper::GetWorldPartitionMiniMap(World, true);
	return 0;
}

bool UExportWorldPartitionMiniMapBuilder::RunInternal(
	UWorld* World,
	const FCellInfo& InCellInfo,
	FPackageSourceControlHelper& PackageHelper)
{
	const FString ExportPath =
		FPaths::ProjectSavedDir() / TEXT("MinimapExports") / FString::Printf(TEXT("%s.png"), *World->GetName());

	FImageWriteOptions Options;
	Options.Format = EDesiredImageFormat::PNG;
	Options.CompressionQuality = 100;
	Options.bOverwriteFile = true;
	Options.bAsync = false;
	Options.NativeOnComplete = [](bool bSuccess) {
		UE_LOG(LogTemp, Log, TEXT("MinimapExport completed -> successful: %s"), *LexToString(bSuccess));
	};

#if USE_CUSTOM_EXPORT_IMPL
	// Custom impl with matching signature
	// Only supports async export!
	OUU::Editor::Private::ExportTexture(WorldMiniMap->MiniMapTexture, ExportPath, Options);
#else
	// Disabled, because it always saves a default 32px texture (see above)

	// This did not change a thing
	// WorldMiniMap->MiniMapTexture->UpdateResource();
	// FlushRenderingCommands();

	UImageWriteBlueprintLibrary::ExportToDisk(WorldMiniMap->MiniMapTexture, ExportPath, Options);
#endif

	return 0;
}

bool UExportWorldPartitionMiniMapBuilder::PostRun(
	UWorld* World,
	FPackageSourceControlHelper& PackageHelper,
	const bool bInRunSuccess)
{
	return 0;
}
