// Copyright Ydrive 2021

#pragma once

#include "CoreMinimal.h"

#include "RendererTargets/RendererTarget.h"


/**
 * Class responsible for updating the world properties before
 * the normal image target rendering and restoring them after the rendering
*/
class FNormalImageTarget : public FRendererTarget
{
public:
    /** Returns the name of the target */
    virtual FString Name() const { return TEXT("NormalImage"); }

    /** Prepares the sequence for rendering the target */
    bool PrepareSequence(ULevelSequence* LevelSequence) override;

    /** Reverts changes made to the sequence by the PrepareSequence */
    bool FinalizeSequence(ULevelSequence* LevelSequence) override;
};
