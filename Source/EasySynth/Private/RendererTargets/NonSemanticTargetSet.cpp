// Copyright (c) 2022 YDrive Inc. All rights reserved.

#include "RendererTargets/NonSemanticTargetSet.h"

#include "Camera/CameraComponent.h"

#include "LevelSequence.h"
#include "TextureStyles/TextureStyleManager.h"


bool FNonSemanticTargetSet::PrepareSequence(ULevelSequence* LevelSequence)
{
	// Update texture style inside the level
	TextureStyleManager->CheckoutTextureStyle(ETextureStyle::COLOR);

	return true;
}

bool FNonSemanticTargetSet::FinalizeSequence(ULevelSequence* LevelSequence)
{
	return true;
}
