// Copyright Epic Games, Inc. All Rights Reserved.

#include "SimpleSynth.h"
#include "SimpleSynthStyle.h"
#include "SimpleSynthCommands.h"
#include "LevelEditor.h"
#include "LevelSequence.h"
#include "PropertyCustomizationHelpers.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "ToolMenus.h"


static const FName SimpleSynthTabName("SimpleSynth");

#define LOCTEXT_NAMESPACE "FSimpleSynthModule"

void FSimpleSynthModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	FSimpleSynthStyle::Initialize();
	FSimpleSynthStyle::ReloadTextures();

	FSimpleSynthCommands::Register();

	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FSimpleSynthCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FSimpleSynthModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FSimpleSynthModule::RegisterMenus));

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(SimpleSynthTabName, FOnSpawnTab::CreateRaw(this, &FSimpleSynthModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FSimpleSynthTabTitle", "SimpleSynth"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FSimpleSynthModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FSimpleSynthStyle::Shutdown();

	FSimpleSynthCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(SimpleSynthTabName);
}

TSharedRef<SDockTab> FSimpleSynthModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	FText WidgetText = FText::Format(
		LOCTEXT("WindowWidgetText", "Add code to {0} in {1} to override this window's contents!"),
		FText::FromString(TEXT("FSimpleSynthModule::OnSpawnPluginTab")),
		FText::FromString(TEXT("SimpleSynth.cpp"))
		);

	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			// Put your tab content here!
			// SNew(SBox)
			// .HAlign(HAlign_Center)
			// .VAlign(VAlign_Center)
			// [
			// 	SNew(STextBlock)
			// 	.Text(WidgetText)
			// ]
			SNew(SScrollBox)
			+SScrollBox::Slot()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("PickSequencer", "Pick sequencer"))
			]
			+SScrollBox::Slot()
			[
				SNew(SObjectPropertyEntryBox)
				.AllowedClass(ULevelSequence::StaticClass())
				// .ObjectPath_UObject(this, &UAssetPickerWidget::GetPath)
				// .OnObjectChanged_UObject(this, &UAssetPickerWidget::OnStaticMeshSelected)
				.AllowClear(true)
				.DisplayUseSelected(true)
				.DisplayBrowse(true)
			]
			+SScrollBox::Slot()
			[
				SNew(SButton)
				// .OnClicked( InKismet2.ToSharedRef(), &FKismet::Compile_OnClicked )
				.Content()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("RenderImages", "Render Images"))
				]
			]
		];
}

void FSimpleSynthModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(SimpleSynthTabName);
}

void FSimpleSynthModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FSimpleSynthCommands::Get().OpenPluginWindow, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("Settings");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FSimpleSynthCommands::Get().OpenPluginWindow));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSimpleSynthModule, SimpleSynth)
