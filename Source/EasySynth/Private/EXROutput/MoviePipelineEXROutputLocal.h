// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "ImageWriteTask.h"
#include "ImagePixelData.h"
#include "MoviePipelineImageSequenceOutput.h"
#include "Misc/StringFormatArg.h"

#if WITH_UNREALEXR
THIRD_PARTY_INCLUDES_START
#include "OpenEXR/include/ImfIO.h"
#include "OpenEXR/include/ImathBox.h"
#include "OpenEXR/include/ImfChannelList.h"
#include "OpenEXR/include/ImfInputFile.h"
#include "OpenEXR/include/ImfOutputFile.h"
#include "OpenEXR/include/ImfArray.h"
#include "OpenEXR/include/ImfHeader.h"
#include "OpenEXR/include/ImfStdIO.h"
#include "OpenEXR/include/ImfChannelList.h"
#include "OpenEXR/include/ImfRgbaFile.h"
THIRD_PARTY_INCLUDES_END
#endif // WITH_UNREALEXR

#include "MoviePipelineEXROutputLocal.generated.h"

UENUM(BlueprintType)
enum class EEXRCompressionFormatLocal : uint8
{
	/** No compression is applied. */
	None,
	/** Good compression quality for grainy images. Lossless.*/
	PIZ,
	/** Good compression quality for images with low amounts of noise. Lossless. */
	ZIP
};

#if WITH_UNREALEXR
class FEXRImageWriteTaskLocal : public IImageWriteTaskBase
{
public:

	/** The filename to write to */
	FString Filename;

	/** True if this task is allowed to overwrite an existing file, false otherwise. */
	bool bOverwriteFile;

	/** Compression method used for the resulting EXR files. */
	EEXRCompressionFormatLocal Compression;

	/** A function to invoke on the game thread when the task has completed */
	TFunction<void(bool)> OnCompleted;

	/** Width/Height of the image data. All samples should match this. */
	int32 Width;

	int32 Height;

	/** A set of key/value pairs to write into the exr file as metadata. */
	FStringFormatNamedArguments FileMetadata;

	/** The image data to write. Supports multiple layers of different bitdepths. */
	TArray<TUniquePtr<FImagePixelData>> Layers;

	/** Optional. A mapping between the FImagePixelData and a name. The standard is that the default layer is nameless (at which point it would be omitted) and other layers are prefixed. */
	TMap<FImagePixelData*, FString> LayerNames;

	FEXRImageWriteTaskLocal()
		: bOverwriteFile(true)
		, Compression(EEXRCompressionFormatLocal::PIZ)
	{}

public:

	virtual bool RunTask() override final;
	virtual void OnAbandoned() override final;

private:

	/**
	 * Run the task, attempting to write out the raw data using the currently specified parameters
	 *
	 * @return true on success, false on any failure
	 */
	bool WriteToDisk();

	/**
	 * Ensures that the desired output filename is writable, deleting an existing file if bOverwriteFile is true
	 *
	 * @return True if the file is writable and the task can proceed, false otherwise
	 */
	bool EnsureWritableFile();

	/**
	* Adds arbitrary key/value pair metadata to the header of the file.
	*/
	void AddFileMetadata(Imf::Header& InHeader);

	template <Imf::PixelType OutputFormat>
	int64 CompressRaw(Imf::Header& InHeader, Imf::FrameBuffer& InFrameBuffer, FImagePixelData* InLayer);
};
#endif // WITH_UNREALEXR

UCLASS()
class UMoviePipelineImageSequenceOutput_EXRLocal : public UMoviePipelineImageSequenceOutputBase
{
	GENERATED_BODY()
public:
#if WITH_EDITOR
	virtual FText GetDisplayText() const override { return NSLOCTEXT("MovieRenderPipeline", "ImgSequenceEXRSettingDisplayName", ".exr Sequence [16bit]"); }
#endif
public:
	UMoviePipelineImageSequenceOutput_EXRLocal()
	{
		OutputFormat = EImageFormat::EXR;
		Compression = EEXRCompressionFormatLocal::PIZ;
		bMultilayer = true;
	}

	virtual void OnReceiveImageDataImpl(FMoviePipelineMergerOutputFrame* InMergedOutputFrame) override;

public:
	/**
	* Which compression method should the resulting EXR file be compressed with
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EXR")
	EEXRCompressionFormatLocal Compression;

	/**
	* Should we write all render passes to the same exr file? Not all software supports multi-layer exr files.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EXR")
	bool bMultilayer;
};
