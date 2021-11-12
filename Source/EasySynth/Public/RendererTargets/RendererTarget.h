// Copyright Ydrive 2021

#pragma once

#include "CoreMinimal.h"

class ULevelSequence;


/**
 * Base class for renderer targets responsible for updating the
 * world properties before a specific target rendering and
 * restoring them after the rendering
*/
class FRendererTarget
{
public:
    /** Returns a name of a specific target */
    virtual FString Name() const = 0;

    /** Prepares the sequence for rendering a specific target */
    virtual bool PrepareSequence(ULevelSequence* LevelSequence) = 0;

    /** Reverts changes made to the sequence by the PrepareSequence */
    virtual bool FinalizeSequence(ULevelSequence* LevelSequence) = 0;
};
