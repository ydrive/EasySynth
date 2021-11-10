// Copyright Ydrive 2021

#pragma once

#include "CoreMinimal.h"

#include "RendererTargets/RendererTarget.h"


/**
 * Class responsible for updating the world properties before
 * the color image target rendering and restoring them after the rendering
*/
class FColorImageTarget : public FRendererTarget
{
public:
    /** Returns the name of the target */
    virtual FString Name() const { return TEXT("ColorImage"); }
};
