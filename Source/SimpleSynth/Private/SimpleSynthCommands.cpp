// Copyright Epic Games, Inc. All Rights Reserved.

#include "SimpleSynthCommands.h"

#define LOCTEXT_NAMESPACE "FSimpleSynthModule"

void FSimpleSynthCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "SimpleSynth", "Bring up SimpleSynth window", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
