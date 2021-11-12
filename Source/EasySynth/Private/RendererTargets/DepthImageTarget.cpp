// Copyright Ydrive 2021

#include "RendererTargets/DepthImageTarget.h"

#include "LevelSequence.h"


bool FDepthImageTarget::PrepareSequence(ULevelSequence* LevelSequence)
{
    TArray<UCameraComponent*> Cameras = GetCameras(LevelSequence);
    UE_LOG(LogEasySynth, Error, TEXT("%s: Cameras num %d"), *FString(__FUNCTION__), Cameras.Num());
    return true;
}

bool FDepthImageTarget::FinalizeSequence(ULevelSequence* LevelSequence)
{
    return true;
}
