// Copyright (c) 2022 YDrive Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

#include "Widgets/WidgetManager.h"

class FToolBarBuilder;
class FMenuBuilder;


// Declare EasySynth log category
DECLARE_LOG_CATEGORY_EXTERN(LogEasySynth, Log, All);


class FEasySynthModule : public IModuleInterface
{
public:
	/** IModuleInterface startup implementation */
	void StartupModule() override;

	/** IModuleInterface shutdown implementation */
	void ShutdownModule() override;

	/** This function will be bound to Command (by default it will bring up plugin window) */
	void PluginButtonClicked();

private:
	/** Adds the plugin to editor menus */
	void RegisterMenus();

private:
	TSharedPtr<class FUICommandList> PluginCommands;

	/** Utility that manages editor tab UI */
	FWidgetManager WidgetManager;
};
