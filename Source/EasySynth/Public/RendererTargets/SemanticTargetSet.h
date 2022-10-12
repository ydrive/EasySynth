// Copyright (c) 2022 YDrive Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"

#include "RendererTargets/RendererTargetSet.h"

class UTextureStyleManager;


/**
 * Class responsible for updating the world properties before
 * the semantic image target rendering and restoring them after the rendering
*/
class FSemanticTargetSet : public FRendererTargetSet
{
public:
	explicit FSemanticTargetSet(UTextureStyleManager* TextureStyleManager, const EImageFormat ImageFormat) :
		FRendererTargetSet(TextureStyleManager, ImageFormat)
	{}

	/** Returns a name of the target set*/
	virtual FString Name() const
	{
		return TEXT("SemanticTargetSet");
	}

	/** Returns the name of the target */
	virtual TArray<FString> TargetNames() const
	{
		TArray<FString> TargetNames;
		TargetNames.Add(TEXT("SemanticImage"));
		return TargetNames;
	}

	/** Prepares the sequence for rendering the target */
	bool PrepareSequence(ULevelSequence* LevelSequence) override;

	/** Reverts changes made to the sequence by the PrepareSequence */
	bool FinalizeSequence(ULevelSequence* LevelSequence) override;
};
