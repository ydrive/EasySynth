// Copyright (c) 2022 YDrive Inc. All rights reserved.

#include "EasySynth.h"
#include "EasySynthStyle.h"
#include "EasySynthCommands.h"
#include "LevelEditor.h"
#include "ToolMenus.h"


static const FName EasySynthTabName("EasySynth");

#define LOCTEXT_NAMESPACE "FEasySynthModule"

void FEasySynthModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	FEasySynthStyle::Initialize();
	FEasySynthStyle::ReloadTextures();

	FEasySynthCommands::Register();

	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FEasySynthCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FEasySynthModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FEasySynthModule::RegisterMenus));

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(EasySynthTabName, FOnSpawnTab::CreateRaw(&WidgetManager, &FWidgetManager::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FEasySynthTabTitle", "EasySynth"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FEasySynthModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FEasySynthStyle::Shutdown();

	FEasySynthCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(EasySynthTabName);
}

void FEasySynthModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(EasySynthTabName);
}

void FEasySynthModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FEasySynthCommands::Get().OpenPluginWindow, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("Settings");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FEasySynthCommands::Get().OpenPluginWindow));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FEasySynthModule, EasySynth)

// Define EasySynth log category
DEFINE_LOG_CATEGORY(LogEasySynth);
