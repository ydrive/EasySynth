// Copyright Ydrive 2021

#include "RendererTargets/SemanticImageTarget.h"

#include "LevelSequence.h"


bool FSemanticImageTarget::PrepareSequence(ULevelSequence* LevelSequence)
{
    TArray<UCameraComponent*> Cameras = GetCameras(LevelSequence);
    UE_LOG(LogEasySynth, Error, TEXT("%s: Cameras num %d"), *FString(__FUNCTION__), Cameras.Num());
    return true;
}

bool FSemanticImageTarget::FinalizeSequence(ULevelSequence* LevelSequence)
{
    return true;
}
