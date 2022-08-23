// Copyright (c) 2022 YDrive Inc. All rights reserved.

#include "Widgets/SemanticClassesWidgetManager.h"

#include "Interfaces/IMainFrameModule.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Colors/SColorPicker.h"
#include "Widgets/Layout/SUniformGridPanel.h"


#define LOCTEXT_NAMESPACE "FSemanticClassesWidgetManager"

FSemanticClassesWidgetManager::FSemanticClassesWidgetManager() :
	NewClassName(FText::GetEmpty()),
	NewClassColor(FColor::White)
{}

FReply FSemanticClassesWidgetManager::OnManageSemanticClassesClicked()
{
	if (!FModuleManager::Get().IsModuleLoaded("MainFrame"))
	{
		UE_LOG(LogEasySynth, Error, TEXT("%s: Failed to load the main frame module"), *FString(__FUNCTION__))
		return FReply::Handled();
	}
	IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");

	// Prepare the classes box
	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);
	ClassesBox = Box;

	// Populate the semantic classes box
	RefreshSemanticClasses();

	// Crate the window
	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(LOCTEXT("ManageSemanticClassesWindowTitle", "Manage Semantic Classes"))
		.SizingRule(ESizingRule::Autosized)
		.SupportsMaximize(false)
		.SupportsMinimize(false)
		.Content()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.Padding(3)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("EditOrRemoveSectionTitle", "Edit or remove semantic classes"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				Box
			]
			+ SVerticalBox::Slot()
			.Padding(3)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("AddNewSemanticClassSectionTitle", "Add new semantic class"))
			]
			+ SVerticalBox::Slot()
			.Padding(3)
			[
				SNew(SEditableTextBox)
				.Text_Lambda([&](){ return NewClassName; })
				.OnTextChanged_Lambda([&](const FText& NewText){ NewClassName = NewText; })
			]
			+ SVerticalBox::Slot()
			.Padding(3)
			[
				SNew(SColorBlock)
				.Color_Lambda([&](){ return NewClassColor; })
				.ShowBackgroundForAlpha(false)
				.IgnoreAlpha(true)
				.OnMouseButtonDown_Raw(this, &FSemanticClassesWidgetManager::OnNewClassColorClicked)
				.Size(FVector2D(35.0f, 17.0f))
			]
			+ SVerticalBox::Slot()
			.Padding(3)
			[
				SNew(SButton)
				.Text(LOCTEXT("AddNewClassButtonText", "Add new class"))
				.OnClicked_Raw(this, &FSemanticClassesWidgetManager::OnAddNewClassClicked)
			]
			+ SVerticalBox::Slot()
			.HAlign(HAlign_Right)
			.Padding(2)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.Text(LOCTEXT("DoneButtonText", "Done"))
				.OnClicked_Raw(this, &FSemanticClassesWidgetManager::OnDoneClicked)
			]
		];
	WidgetWindow = Window;

	TSharedPtr<SWindow> ParentWindow = MainFrame.GetParentWindow();
	if (!ParentWindow.IsValid())
	{
		UE_LOG(LogEasySynth, Error, TEXT("%s: Failed to get the parent window"), *FString(__FUNCTION__))
		return FReply::Handled();
	}
	FSlateApplication::Get().AddModalWindow(Window, ParentWindow, false);

	return FReply::Handled();
}

void FSemanticClassesWidgetManager::OnClassNameChanged(
	const FText& NewText,
	ETextCommit::Type CommitType,
	const FString ClassName)
{
	const bool bSuccess = TextureStyleManager->UpdateClassName(ClassName, NewText.ToString());
	if (bSuccess)
	{
		RefreshSemanticClasses();
	}
}

FReply FSemanticClassesWidgetManager::OnUpdateClassColorClicked(
	const FGeometry& MyGeometry,
	const FPointerEvent& MouseEvent,
	const FString ClassName)
{
	CurrentlyEditedClass = ClassName;

	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Handled();
	}

	FColorPickerArgs PickerArgs;
	{
		PickerArgs.bUseAlpha = false;
		PickerArgs.bOnlyRefreshOnOk = true;
		PickerArgs.DisplayGamma = TAttribute<float>::Create(TAttribute<float>::FGetter::CreateUObject(GEngine, &UEngine::GetDisplayGamma));
		PickerArgs.OnColorCommitted = FOnLinearColorValueChanged::CreateRaw(this, &FSemanticClassesWidgetManager::OnUpdateClassColorCommited);
		PickerArgs.OnColorPickerWindowClosed = FOnWindowClosed::CreateRaw(this, &FSemanticClassesWidgetManager::OnColorPickerWindowClosed);
		PickerArgs.InitialColorOverride = FLinearColor(TextureStyleManager->ClassColor(ClassName));
	}

	// Need to close the current window to be able to display the color picker
	if (WidgetWindow.IsValid())
	{
		WidgetWindow.Pin()->RequestDestroyWindow();
	}

	OpenColorPicker(PickerArgs);

	return FReply::Handled();
}

void FSemanticClassesWidgetManager::OnUpdateClassColorCommited(const FLinearColor NewLinearColor)
{
	const bool bSRGB = true;
	FColor NewColor = NewLinearColor.ToFColor(bSRGB);
	NewColor.A = 255;

	TextureStyleManager->UpdateClassColor(CurrentlyEditedClass, NewColor);

	CurrentlyEditedClass = "";
}

FReply FSemanticClassesWidgetManager::OnDeleteClassClicked(const FString ClassName)
{
	const bool bSuccess = TextureStyleManager->RemoveSemanticClass(ClassName);
	if (bSuccess)
	{
		RefreshSemanticClasses();
	}
	return FReply::Handled();
}

FReply FSemanticClassesWidgetManager::OnNewClassColorClicked(
	const FGeometry& MyGeometry,
	const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Handled();
	}

	FColorPickerArgs PickerArgs;
	{
		PickerArgs.bUseAlpha = false;
		PickerArgs.bOnlyRefreshOnOk = true;
		PickerArgs.DisplayGamma = TAttribute<float>::Create(TAttribute<float>::FGetter::CreateUObject(GEngine, &UEngine::GetDisplayGamma));
		PickerArgs.OnColorCommitted = FOnLinearColorValueChanged::CreateRaw(this, &FSemanticClassesWidgetManager::OnNewClassColorCommited);
		PickerArgs.OnColorPickerWindowClosed = FOnWindowClosed::CreateRaw(this, &FSemanticClassesWidgetManager::OnColorPickerWindowClosed);
		PickerArgs.InitialColorOverride = FLinearColor(NewClassColor);
	}

	// Need to close the current window to be able to display the color picker
	if (WidgetWindow.IsValid())
	{
		WidgetWindow.Pin()->RequestDestroyWindow();
	}

	OpenColorPicker(PickerArgs);

	return FReply::Handled();
}

void FSemanticClassesWidgetManager::OnNewClassColorCommited(const FLinearColor NewLinearColor)
{
	const bool bSRGB = true;
	NewClassColor = NewLinearColor.ToFColor(bSRGB);
	NewClassColor.A = 255;
}

FReply FSemanticClassesWidgetManager::OnAddNewClassClicked()
{
	const bool bSuccess = TextureStyleManager->NewSemanticClass(NewClassName.ToString(), NewClassColor);
	if (bSuccess)
	{
		NewClassName = FText::GetEmpty();
		NewClassColor = FColor::White;
		RefreshSemanticClasses();
	}

	return FReply::Handled();
}

void FSemanticClassesWidgetManager::OnColorPickerWindowClosed(const TSharedRef<SWindow>& Window)
{
	// Reopen the semantic classes window
	// Make a brief delay to enable the color picker to close
	const float DelaySeconds = 0.2f;
	const bool bLoop = false;
	GEditor->GetEditorWorldContext().World()->GetTimerManager().SetTimer(
		ReopenTimerHandle,
		[this](){ OnManageSemanticClassesClicked(); },
		DelaySeconds,
		bLoop);
}

FReply FSemanticClassesWidgetManager::OnDoneClicked()
{
	if (WidgetWindow.IsValid())
	{
		WidgetWindow.Pin()->RequestDestroyWindow();
	}
	else
	{
		UE_LOG(LogEasySynth, Error, TEXT("%s: Widget window is invalid"), *FString(__FUNCTION__))
	}

	return FReply::Handled();
}

void FSemanticClassesWidgetManager::RefreshSemanticClasses()
{
	if (!ClassesBox.IsValid())
	{
		UE_LOG(LogEasySynth, Error, TEXT("%s: Invalid classes box"), *FString(__FUNCTION__))
		return;
	}

	ClassesBox.Pin()->ClearChildren();

	TArray<const FSemanticClass*> SemanticClasses = TextureStyleManager->SemanticClasses();
	for (int i = 0; i < SemanticClasses.Num(); i++)
	{
		const FSemanticClass* SemanticClass = SemanticClasses[i];
		ClassesBox.Pin()->AddSlot()
		.Padding(6)
		[
			SNew(SVerticalBox)
			.IsEnabled_Lambda([i](){ return i > 0; })
			+ SVerticalBox::Slot()
			.Padding(2)
			[
				SNew(SEditableTextBox)
				.Text_Lambda([SemanticClass](){ return FText::FromString(SemanticClass->Name); })
				.OnTextCommitted_Raw(this, &FSemanticClassesWidgetManager::OnClassNameChanged, SemanticClass->Name)
			]
			+ SVerticalBox::Slot()
			.Padding(2)
			[
				SNew(SColorBlock)
				.Color_Lambda([SemanticClass](){ return SemanticClass->Color; })
				.ShowBackgroundForAlpha(false)
				.IgnoreAlpha(true)
				.OnMouseButtonDown_Raw(this, &FSemanticClassesWidgetManager::OnUpdateClassColorClicked, SemanticClass->Name)
			]
			+ SVerticalBox::Slot()
			.Padding(2)
			[
				SNew(SButton)
				.Text(LOCTEXT("DeleteClassButtonText", "Delete"))
				.OnClicked_Raw(this, &FSemanticClassesWidgetManager::OnDeleteClassClicked, SemanticClass->Name)
			]
		];
	}
}

#undef LOCTEXT_NAMESPACE
