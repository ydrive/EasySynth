// Copyright Ydrive 2021

#pragma once

#include "CoreMinimal.h"


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
};
