// Copyright (c) YDrive Inc. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "RendererTargets/RendererTarget.h"

class UTextureStyleManager;


/**
 * Class responsible for updating the world properties before
 * the semantic image target rendering and restoring them after the rendering
*/
class FSemanticImageTarget : public FRendererTarget
{
public:
	explicit FSemanticImageTarget(UTextureStyleManager* TextureStyleManager, const bool bUseExr) :
		FRendererTarget(TextureStyleManager, bUseExr)
	{}

	/** Returns the name of the target */
	virtual FString Name() const { return TEXT("SemanticImage"); }

	/** Prepares the sequence for rendering the target */
	bool PrepareSequence(ULevelSequence* LevelSequence) override;

	/** Reverts changes made to the sequence by the PrepareSequence */
	bool FinalizeSequence(ULevelSequence* LevelSequence) override;
};
