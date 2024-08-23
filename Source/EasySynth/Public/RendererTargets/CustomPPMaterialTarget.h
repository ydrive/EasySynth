// Copyright (c) 2024 YDrive Inc. All rights reserved.
#pragma once

#include "CoreMinimal.h"

#include "RendererTargets/RendererTarget.h"

class UTextureStyleManager;


/**
 * Class responsible for updating the world properties before the custom PP
 * material target rendering and restoring them after the rendering
*/
class FCustomPPMaterialTarget : public FRendererTarget
{
public:
	explicit FCustomPPMaterialTarget(
			UTextureStyleManager* TextureStyleManager,
			const EImageFormat ImageFormat,
			UMaterial* CustomPPMaterial) :
		FRendererTarget(TextureStyleManager, ImageFormat),
		CustomPPMaterial(CustomPPMaterial)
	{}

	/** Returns the name of the target */
	virtual FString Name() const { return TEXT("CustomPPMaterial"); }

	/** Prepares the sequence for rendering the target */
	bool PrepareSequence(ULevelSequence* LevelSequence) override;

	/** Reverts changes made to the sequence by the PrepareSequence */
	bool FinalizeSequence(ULevelSequence* LevelSequence) override;

private:
	UMaterial* CustomPPMaterial;
};
