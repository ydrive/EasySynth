// Copyright Ydrive 2021

#include "RendererTargets/NormalImageTarget.h"

#include "LevelSequence.h"


bool FNormalImageTarget::PrepareSequence(ULevelSequence* LevelSequence)
{
    TArray<UCameraComponent*> Cameras = GetCameras(LevelSequence);
    UE_LOG(LogEasySynth, Error, TEXT("%s: Cameras num %d"), *FString(__FUNCTION__), Cameras.Num());
    return true;
}

bool FNormalImageTarget::FinalizeSequence(ULevelSequence* LevelSequence)
{
    return true;
}
