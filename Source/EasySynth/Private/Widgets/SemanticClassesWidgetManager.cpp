// Copyright (c) 2022 YDrive Inc. All rights reserved.

#include "Widgets/SemanticClassesWidgetManager.h"

#include "Interfaces/IMainFrameModule.h"
#include "Misc/MessageDialog.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Colors/SColorPicker.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"

#include "EasySynth.h"
#include "TextureStyles/TextureMappingAsset.h"
#include "TextureStyles/TextureStyleManager.h"


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
		.SizingRule(ESizingRule::UserSized)
		.ClientSize(FVector2D(460.0f, 520.0f))
		.SupportsMaximize(false)
		.SupportsMinimize(false)
		.Content()
		[
			SNew(SVerticalBox)

			// Section heading for the existing classes
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(10.0f, 10.0f, 10.0f, 4.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("EditOrRemoveSectionTitle", "Edit or remove semantic classes"))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
			]

			// Column headers for the class list
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(12.0f, 0.0f, 12.0f, 2.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("NameColumnHeader", "Name"))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(8.0f, 0.0f)
				.VAlign(VAlign_Center)
				[
					SNew(SBox)
					.WidthOverride(40.0f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("ColorColumnHeader", "Color"))
					]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SBox)
					.WidthOverride(70.0f)
				]
			]

			// Scrollable list of existing classes, so the window never grows past its bounds
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(10.0f, 0.0f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
				.Padding(4.0f)
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						Box
					]
				]
			]

			// Section heading for adding a new class
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(10.0f, 10.0f, 10.0f, 4.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("AddNewSemanticClassSectionTitle", "Add new semantic class"))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
			]

			// Inline row for entering a new class name, color and confirming
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(12.0f, 0.0f, 12.0f, 6.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				[
					SNew(SEditableTextBox)
					.Text_Lambda([&](){ return NewClassName; })
					.HintText(LOCTEXT("NewClassNameHint", "Class name"))
					.OnTextChanged_Lambda([&](const FText& NewText){ NewClassName = NewText; })
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(8.0f, 0.0f)
				.VAlign(VAlign_Center)
				[
					SNew(SColorBlock)
					.Color_Lambda([&](){ return NewClassColor; })
					.ShowBackgroundForAlpha(false)
					.AlphaDisplayMode(EColorBlockAlphaDisplayMode::Ignore)
					.OnMouseButtonDown_Raw(this, &FSemanticClassesWidgetManager::OnNewClassColorClicked)
					.Size(FVector2D(40.0f, 20.0f))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SBox)
					.WidthOverride(70.0f)
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.Text(LOCTEXT("AddNewClassButtonText", "Add"))
						.OnClicked_Raw(this, &FSemanticClassesWidgetManager::OnAddNewClassClicked)
					]
				]
			]

			// Separator above the dialog actions
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(10.0f, 4.0f)
			[
				SNew(SSeparator)
			]

			// Done button
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			.Padding(10.0f, 4.0f, 10.0f, 10.0f)
			[
				SNew(SBox)
				.WidthOverride(90.0f)
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.Text(LOCTEXT("DoneButtonText", "Done"))
					.OnClicked_Raw(this, &FSemanticClassesWidgetManager::OnDoneClicked)
				]
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
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Handled();
	}

	// Changing an existing class color while the semantic texture style is active immediately
	// repaints the affected actors, which is unstable and crashes the editor. Require the user
	// to switch the mesh texture style back to the original color view before editing colors.
	if (TextureStyleManager != nullptr &&
		TextureStyleManager->SelectedTextureStyle() == ETextureStyle::SEMANTIC)
	{
		FMessageDialog::Open(
			EAppMsgType::Ok,
			LOCTEXT(
				"CannotEditColorInSemanticMode",
				"Semantic class colors cannot be changed while the semantic texture style is active.\n\n"
				"Switch the mesh texture style back to the original color view, then change the color."));
		return FReply::Handled();
	}

	CurrentlyEditedClass = ClassName;

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

		// The first class is the default background class, which must not be edited or removed
		const bool bIsEditable = i > 0;

		ClassesBox.Pin()->AddSlot()
		.AutoHeight()
		.Padding(2.0f)
		[
			SNew(SHorizontalBox)
			.IsEnabled(bIsEditable)

			// Class name
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SEditableTextBox)
				.Text_Lambda([SemanticClass](){ return FText::FromString(SemanticClass->Name); })
				.OnTextCommitted_Raw(this, &FSemanticClassesWidgetManager::OnClassNameChanged, SemanticClass->Name)
			]

			// Class color
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(8.0f, 0.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SColorBlock)
				.Color_Lambda([SemanticClass](){ return SemanticClass->Color; })
				.ShowBackgroundForAlpha(false)
				.AlphaDisplayMode(EColorBlockAlphaDisplayMode::Ignore)
				.OnMouseButtonDown_Raw(this, &FSemanticClassesWidgetManager::OnUpdateClassColorClicked, SemanticClass->Name)
				.Size(FVector2D(40.0f, 20.0f))
			]

			// Delete button
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SBox)
				.WidthOverride(70.0f)
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.Text(LOCTEXT("DeleteClassButtonText", "Delete"))
					.OnClicked_Raw(this, &FSemanticClassesWidgetManager::OnDeleteClassClicked, SemanticClass->Name)
				]
			]
		];
	}
}

#undef LOCTEXT_NAMESPACE
