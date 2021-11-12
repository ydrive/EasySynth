// Copyright Ydrive 2021

#include "RendererTargets/ColorImageTarget.h"

#include "LevelSequence.h"


bool FColorImageTarget::PrepareSequence(ULevelSequence* LevelSequence)
{
    TArray<UCameraComponent*> Cameras = GetCameras(LevelSequence);
    UE_LOG(LogEasySynth, Error, TEXT("%s: Cameras num %d"), *FString(__FUNCTION__), Cameras.Num());
    return true;
}

bool FColorImageTarget::FinalizeSequence(ULevelSequence* LevelSequence)
{
    return true;
}
