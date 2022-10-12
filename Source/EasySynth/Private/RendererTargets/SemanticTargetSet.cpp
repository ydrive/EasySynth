// Copyright (c) 2022 YDrive Inc. All rights reserved.

#include "RendererTargets/SemanticTargetSet.h"

#include "Camera/CameraComponent.h"

#include "LevelSequence.h"
#include "TextureStyles/TextureStyleManager.h"


bool FSemanticTargetSet::PrepareSequence(ULevelSequence* LevelSequence)
{
	// Update texture style inside the level
	TextureStyleManager->CheckoutTextureStyle(ETextureStyle::SEMANTIC);

	return true;
}

bool FSemanticTargetSet::FinalizeSequence(ULevelSequence* LevelSequence)
{
	return true;
}
