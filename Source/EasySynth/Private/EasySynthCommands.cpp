// Copyright Ydrive 2021

#include "EasySynthCommands.h"

#define LOCTEXT_NAMESPACE "FEasySynthModule"

void FEasySynthCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "EasySynth", "Bring up EasySynth window", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
