// Copyright (c) 2022 YDrive Inc. All rights reserved.

#include "TextureStyles/TextureStyleManager.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/StaticMeshComponent.h"
#include "EditorAssetLibrary.h"
#include "Engine/Selection.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "FileHelpers.h"
#include "HAL/FileManagerGeneric.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceConstant.h"

#include "PathUtils.h"
#include "TextureStyles/TextureBackupManager.h"
#include "TextureStyles/TextureMappingAsset.h"


const FString UTextureStyleManager::SemanticColorParameter(TEXT("SemanticColor"));
const FString UTextureStyleManager::UndefinedSemanticClassName(TEXT("Undefined"));

UTextureStyleManager::UTextureStyleManager() :
	PlainColorMaterial(DuplicateObject<UMaterial>(
		LoadObject<UMaterial>(nullptr, *FPathUtils::PlainColorMaterialPath()), nullptr)),
	CurrentTextureStyle(ETextureStyle::COLOR),
	TextureBackupManager(NewObject<UTextureBackupManager>()),
	bEventsBound(false)
{
	// Check if the plain color material is loaded correctly
	if (PlainColorMaterial == nullptr)
	{
		UE_LOG(LogEasySynth, Error, TEXT("%s: Could not load the PlainColorMaterial"), *FString(__FUNCTION__))
		check(PlainColorMaterial)
	}

	// Check if the TextureBackupManager is initialized correctly
	if (TextureBackupManager == nullptr)
	{
		UE_LOG(LogEasySynth, Error, TEXT("%s: Could not create the TextureBackupManager"), *FString(__FUNCTION__))
		check(TextureBackupManager)
	}

	// Prepare the texture mapping asset
	LoadOrCreateTextureMappingAsset();
}

void UTextureStyleManager::BindEvents()
{
	// Bind event to OnLevelActorDeleted event to remove the actor from our buffers
	if (!bEventsBound)
	{
		GEngine->OnLevelActorAdded().AddUObject(this, &UTextureStyleManager::OnLevelActorAdded);
		GEngine->OnLevelActorDeleted().AddUObject(this, &UTextureStyleManager::OnLevelActorDeleted);
		GEngine->OnEditorClose().AddUObject(this, &UTextureStyleManager::OnEditorClose);
		bEventsBound = true;
	}
}

bool UTextureStyleManager::NewSemanticClass(
	const FString& ClassName,
	const FColor& ClassColor,
	const bool bSaveTextureMappingAsset)
{
	if (ClassName.Len() == 0)
	{
		UE_LOG(LogEasySynth, Warning, TEXT("%s: Cannot create a class with the name ''"), *FString(__FUNCTION__));
		return false;
	}

	// Check collisions with existing classes
	for (auto& Element : TextureMappingAsset->SemanticClasses)
	{
		const FSemanticClass& SemanticClass = Element.Value;
		if (SemanticClass.Name == ClassName || SemanticClass.Color == ClassColor)
		{
			UE_LOG(LogEasySynth, Warning, TEXT("%s: New semantic class (%s, (%d %d %d)) colliding with existing (%s, (%d %d %d))"),
				*FString(__FUNCTION__),
				*ClassName, ClassColor.R, ClassColor.G, ClassColor.B,
				*SemanticClass.Name, SemanticClass.Color.R, SemanticClass.Color.G, SemanticClass.Color.B);
			return false;
		}
	}

	// Crate the new class
	FSemanticClass& NewSemanticClass = TextureMappingAsset->SemanticClasses.Add(ClassName);
	NewSemanticClass.Name = ClassName;
	NewSemanticClass.Color = ClassColor;
	// The semantic class material instance will be created when it's needed

	if (bSaveTextureMappingAsset)
	{
		SaveTextureMappingAsset();
	}

	// Broadcast the semantic classes change
	SemanticClassesUpdatedEvent.Broadcast();

	return true;
}

FColor UTextureStyleManager::ClassColor(const FString& ClassName)
{
	if (TextureMappingAsset->SemanticClasses.Contains(ClassName))
	{
		return TextureMappingAsset->SemanticClasses[ClassName].Color;
	}
	return FColor::White;
}

bool UTextureStyleManager::UpdateClassName(const FString& OldClassName, const FString& NewClassName)
{
	if (OldClassName == NewClassName)
	{
		return true;
	}

	if (!TextureMappingAsset->SemanticClasses.Contains(OldClassName))
	{
		UE_LOG(LogEasySynth, Log, TEXT("%s: Previous semantic class '%s' not found"),
			*FString(__FUNCTION__), *OldClassName);
		return false;
	}

	if (TextureMappingAsset->SemanticClasses.Contains(NewClassName))
	{
		UE_LOG(LogEasySynth, Log, TEXT("%s: New semantic class '%s' already exists"),
			*FString(__FUNCTION__), *NewClassName);
		return false;
	}

	if (NewClassName.Len() == 0)
	{
		UE_LOG(LogEasySynth, Warning, TEXT("%s: Cannot update class name to ''"), *FString(__FUNCTION__));
		return false;
	}

	const FColor ClassColor = TextureMappingAsset->SemanticClasses[OldClassName].Color;

	// Remove the existing class
	TextureMappingAsset->SemanticClasses.Remove(OldClassName);
	// Add new class with the same color
	NewSemanticClass(NewClassName, ClassColor);
	// Update actor mappings to the new semantic class name
	TArray<AActor*> LevelActors;
	UGameplayStatics::GetAllActorsOfClass(GEditor->GetEditorWorldContext().World(), AActor::StaticClass(), LevelActors);
	for (AActor* Actor : LevelActors)
	{
		if (TextureMappingAsset->ActorClassPairs.Contains(Actor->GetActorGuid()) &&
			TextureMappingAsset->ActorClassPairs[Actor->GetActorGuid()] == OldClassName)
		{
			TextureMappingAsset->ActorClassPairs[Actor->GetActorGuid()] = NewClassName;
		}
	}
	// No action regarding actor materials necessary

	SaveTextureMappingAsset();

	return true;
}

bool UTextureStyleManager::UpdateClassColor(const FString& ClassName, const FColor& NewClassColor)
{
	if (!TextureMappingAsset->SemanticClasses.Contains(ClassName))
	{
		UE_LOG(LogEasySynth, Log, TEXT("%s: Requested semantic class '%s' not found"),
			*FString(__FUNCTION__), *ClassName);
		return false;
	}

	if (TextureMappingAsset->SemanticClasses[ClassName].Color == NewClassColor)
	{
		return true;
	}

	// Check if color is already in use
	for (auto& Element : TextureMappingAsset->SemanticClasses)
	{
		const FSemanticClass& SemanticClass = Element.Value;
		if (SemanticClass.Color == NewClassColor)
		{
			UE_LOG(LogEasySynth, Warning, TEXT("%s: Requested color (%d %d %d) already used by %s"),
				*FString(__FUNCTION__), NewClassColor.R, NewClassColor.G, NewClassColor.B, *SemanticClass.Name);
			return false;
		}
	}

	// Update the class color
	TextureMappingAsset->SemanticClasses[ClassName].Color = NewClassColor;
	// Invalidate the material instance
	TextureMappingAsset->SemanticClasses[ClassName].PlainColorMaterialInstance = nullptr;
	// Update each actor color immediately in case of the semantic view mode
	TArray<AActor*> LevelActors;
	UGameplayStatics::GetAllActorsOfClass(GEditor->GetEditorWorldContext().World(), AActor::StaticClass(), LevelActors);
	for (AActor* Actor : LevelActors)
	{
		if (TextureMappingAsset->ActorClassPairs.Contains(Actor->GetActorGuid()) &&
			TextureMappingAsset->ActorClassPairs[Actor->GetActorGuid()] == ClassName)
		{
			SetSemanticClassToActor(Actor, ClassName);
		}
	}

	SaveTextureMappingAsset();

	return true;
}

bool UTextureStyleManager::RemoveSemanticClass(const FString& ClassName)
{
	if (!TextureMappingAsset->SemanticClasses.Contains(ClassName))
	{
		UE_LOG(LogEasySynth, Log, TEXT("%s: Requested semantic class '%s' not found"),
			*FString(__FUNCTION__), *ClassName);
		return false;
	}

	if (ClassName == UndefinedSemanticClassName)
	{
		// Just ignore removing the necessary undefined semantic class
		return true;
	}

	// Reset all actor to the undefined class
	TArray<AActor*> LevelActors;
	UGameplayStatics::GetAllActorsOfClass(GEditor->GetEditorWorldContext().World(), AActor::StaticClass(), LevelActors);
	for (AActor* Actor : LevelActors)
	{
		if (TextureMappingAsset->ActorClassPairs.Contains(Actor->GetActorGuid()) &&
			TextureMappingAsset->ActorClassPairs[Actor->GetActorGuid()] == ClassName)
		{
			SetSemanticClassToActor(Actor, UndefinedSemanticClassName);
		}
	}

	// Remove the class
	TextureMappingAsset->SemanticClasses.Remove(ClassName);

	SaveTextureMappingAsset();

	// Broadcast the semantic classes change
	SemanticClassesUpdatedEvent.Broadcast();

	return true;
}

void UTextureStyleManager::RemoveAllSemanticCLasses()
{
	for (auto& Element : TextureMappingAsset->SemanticClasses)
	{
		const FString& ClassName = Element.Key;
		// Skip the default undefined semantic class
		if (ClassName != UndefinedSemanticClassName)
		{
			RemoveSemanticClass(ClassName);
		}
	}
}

TArray<FString> UTextureStyleManager::SemanticClassNames() const
{
	TArray<FString> SemanticClassNames;
	for (auto& Element : TextureMappingAsset->SemanticClasses)
	{
		SemanticClassNames.Add(Element.Key);
	}
	return SemanticClassNames;
}

TArray<const FSemanticClass*> UTextureStyleManager::SemanticClasses() const
{
	TArray<const FSemanticClass*> SemanticClasses;
	for (auto& Element : TextureMappingAsset->SemanticClasses)
	{
		SemanticClasses.Add(&Element.Value);
	}
	return SemanticClasses;
}

void UTextureStyleManager::ApplySemanticClassToSelectedActors(const FString& ClassName)
{
	if (!TextureMappingAsset->SemanticClasses.Contains(ClassName))
	{
		UE_LOG(LogEasySynth, Warning, TEXT("%s: Received semantic class '%s' not found"),
			*FString(__FUNCTION__), *ClassName);
		return;
	}

	UE_LOG(LogEasySynth, Log, TEXT("%s: Setting the '%s' semantic class to selected actors"),
		*FString(__FUNCTION__), *ClassName)

	TArray<UObject*> SelectedActors;
	GEditor->GetSelectedActors()->GetSelectedObjects(AActor::StaticClass(), SelectedActors);

	for (UObject* SelectedObject : SelectedActors)
	{
		AActor* SelectedActor = Cast<AActor>(SelectedObject);
		if (SelectedActor == nullptr)
		{
			UE_LOG(LogEasySynth, Log, TEXT("%s: Got null actor"), *FString(__FUNCTION__))
			return;
		}

		// Set the class to the actor
		SetSemanticClassToActor(SelectedActor, ClassName);
	}

	SaveTextureMappingAsset();
}

void UTextureStyleManager::CheckoutTextureStyle(const ETextureStyle NewTextureStyle)
{
	UE_LOG(LogEasySynth, Log, TEXT("%s: New texture style: %d"), *FString(__FUNCTION__), NewTextureStyle)

	if (NewTextureStyle == CurrentTextureStyle)
	{
		// Return if the desired style is already selected
		UE_LOG(LogEasySynth, Log, TEXT("%s: TextureStyle %d already selected"), *FString(__FUNCTION__), NewTextureStyle)
		return;
	}

	// Apply materials to all actors
	TArray<AActor*> LevelActors;
	UGameplayStatics::GetAllActorsOfClass(GEditor->GetEditorWorldContext().World(), AActor::StaticClass(), LevelActors);
	for (AActor* Actor : LevelActors)
	{
		CheckoutActorTexture(Actor, NewTextureStyle);
	}

	// Make sure any changes to the TextureMappingAsset are changed
	SaveTextureMappingAsset();

	CurrentTextureStyle = NewTextureStyle;
}

bool UTextureStyleManager::ExportSemanticClasses(const FString& OutputDir)
{
	FSemanticCsvInterface SemanticCsvInterface;
	return SemanticCsvInterface.ExportSemanticClasses(OutputDir, TextureMappingAsset);
}

void UTextureStyleManager::LoadOrCreateTextureMappingAsset()
{
	// Try to load
	TextureMappingAsset = LoadObject<UTextureMappingAsset>(nullptr, *FPathUtils::TextureMappingAssetPath());

	if (TextureMappingAsset == nullptr)
	{
		UE_LOG(LogEasySynth, Log, TEXT("%s: Texture mapping asset not found, creating a new one"),
			*FString(__FUNCTION__));

		// Register the plugin directory with the editor
		FAssetRegistryModule& AssetRegistryModule =
			FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		AssetRegistryModule.Get().AddPath(FPathUtils::ProjectPluginContentDir());

		// Create and populate the asset
		UPackage *TextureMappingPackage = CreatePackage(*FPathUtils::TextureMappingAssetPath());
		check(TextureMappingPackage)
		TextureMappingAsset = NewObject<UTextureMappingAsset>(
			TextureMappingPackage,
			UTextureMappingAsset::StaticClass(),
			*FPathUtils::TextureMappingAssetName,
			EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);
		check(TextureMappingAsset)

		// Initialize the default undefined semantic class with pure white color
		NewSemanticClass(UndefinedSemanticClassName, FColor(255, 255, 255, 255), false);

		// Don't save the asset yet to prevent crashing the editor on startup
	}
}

void UTextureStyleManager::SaveTextureMappingAsset()
{
	check(TextureMappingAsset)
	const bool bOnlyIfIsDirty = false;
	UEditorAssetLibrary::SaveLoadedAsset(TextureMappingAsset, bOnlyIfIsDirty);
}

void UTextureStyleManager::OnLevelActorAdded(AActor* Actor)
{
	UE_LOG(LogEasySynth, Log, TEXT("%s: Adding actor '%s'"), *FString(__FUNCTION__), *Actor->GetName())

	// Preemptively assign the undefined semantic class to the new actor
	// In the case of the semantic mode being selected, assigned class will be immediately displayed
	const bool bForceDisplaySemanticClass = false;
	const bool bDelayAddingDescriptors = true;
	SetSemanticClassToActor(Actor, UndefinedSemanticClassName, bForceDisplaySemanticClass, bDelayAddingDescriptors);
}

void UTextureStyleManager::OnLevelActorDeleted(AActor* Actor)
{
	UE_LOG(LogEasySynth, Log, TEXT("%s: Removing actor '%s'"), *FString(__FUNCTION__), *Actor->GetName())
	TextureMappingAsset->ActorClassPairs.Remove(Actor->GetActorGuid());
	TextureBackupManager->RemoveActor(Actor);
}

void UTextureStyleManager::OnEditorClose()
{
	UE_LOG(LogEasySynth, Log, TEXT("%s: Making sure original mesh colors are selected"), *FString(__FUNCTION__))
	CheckoutTextureStyle(ETextureStyle::COLOR);
	// Make level dirty and save it
	ULevel* Level = GWorld->GetCurrentLevel();
	Level->MarkPackageDirty();
	FEditorFileUtils::SaveLevel(Level);
}

void UTextureStyleManager::SetSemanticClassToActor(
	AActor* Actor,
	const FString& ClassName,
	const bool bForceDisplaySemanticClass,
	const bool bDelayAddingDescriptors)
{
	// Remove class if already assigned
	TextureMappingAsset->ActorClassPairs.Remove(Actor->GetActorGuid());

	// Set the new class
	TextureMappingAsset->ActorClassPairs.Add(Actor->GetActorGuid(), ClassName);

	// Immediately display the change when in the semantic mode
	if (CurrentTextureStyle == ETextureStyle::SEMANTIC)
	{
		if (bDelayAddingDescriptors)
		{
			// Adding with a delay is requested, add the actor to the delay buffer, start the timer,
			// stop the current execution and the callback will handle the rest
			DelayActorBuffer.Add(Actor);
			const float DelaySeconds = 0.2f;
			const bool bLoop = false;
			GEditor->GetEditorWorldContext().World()->GetTimerManager().SetTimer(
				DelayActorTimerHandle,
				this,
				&UTextureStyleManager::ProcessDelayActorBuffer,
				DelaySeconds,
				bLoop);
			return;
		}
	}

	// No delay is requested, checkout the semantic color
	if (bForceDisplaySemanticClass || CurrentTextureStyle == ETextureStyle::SEMANTIC)
	{
		CheckoutActorTexture(Actor, ETextureStyle::SEMANTIC);
	}

	// No need to save the TextureMappingAsset for every actor, the caller will do it
}

void UTextureStyleManager::CheckoutActorTexture(AActor* Actor, const ETextureStyle NewTextureStyle)
{
	// Check if the actor has a semantic class assigned
	const FGuid& ActorGuid = Actor->GetActorGuid();
	if (!TextureMappingAsset->ActorClassPairs.Contains(ActorGuid))
	{
		if (NewTextureStyle == ETextureStyle::SEMANTIC)
		{
			// If semantic view is being selected, assign the default class to the actor
			// This method will be recalled by the following method
			const bool bForceDisplaySemanticClass = true;
			SetSemanticClassToActor(Actor, UndefinedSemanticClassName, bForceDisplaySemanticClass);
		}
		return;
	}

	// Get the name of the semantic class assigned to the actor
	const FString& ClassName = TextureMappingAsset->ActorClassPairs[ActorGuid];

	// Make sure the semantic class name is valid
	if (!TextureMappingAsset->SemanticClasses.Contains(ClassName))
	{
		UE_LOG(LogEasySynth, Error, TEXT("%s: Uknown class name '%s'"), *FString(__FUNCTION__), *ClassName)
		return;
	}

	// Check whether the actor currently has its original materials active
	// It it does, the original materials will be backed up
	const bool bOriginalTextureActive = !TextureBackupManager->ContainsActor(Actor);

	// If the actor has original texture and is changing to the same one, ignore the actor
	if (bOriginalTextureActive && NewTextureStyle == ETextureStyle::COLOR)
	{
		return;
	}

	// Update the actor texture
	const bool bDoAdd = bOriginalTextureActive;
	const bool bDoPaint = true;
	UMaterialInstanceConstant* Material = nullptr;
	if (NewTextureStyle == ETextureStyle::SEMANTIC)
	{
		Material = GetSemanticClassMaterial(TextureMappingAsset->SemanticClasses[ClassName]);
	}
	TextureBackupManager->AddAndPaint(Actor, bDoAdd, bDoPaint, Material);
}

void UTextureStyleManager::ProcessDelayActorBuffer()
{
	bool bAnyActorProcessed = false;
	for (AActor* Actor : DelayActorBuffer)
	{
		if (IsValid(Actor))
		{
			// Must not call with bDelayAddingDescriptors = true, to avoid infinite recursion
			SetSemanticClassToActor(Actor, UndefinedSemanticClassName);
			bAnyActorProcessed = true;
		}
	}
	DelayActorBuffer.Empty();
	if (bAnyActorProcessed)
	{
		SaveTextureMappingAsset();
	}
}

UMaterialInstanceConstant* UTextureStyleManager::GetSemanticClassMaterial(FSemanticClass& SemanticClass)
{
	// If the plain color material is null, create it
	if (SemanticClass.PlainColorMaterialInstance == nullptr)
	{
		UMaterialInstanceConstantFactoryNew* Factory = NewObject<UMaterialInstanceConstantFactoryNew>();
		Factory->InitialParent = PlainColorMaterial;

		const FString PackageFileName = FString::Printf(TEXT("M_%s"), *(SemanticClass.Name));
		const FString PackagePath =
			FPathUtils::ProjectPluginContentDir() / FString(TEXT("ConstMaterials")) / PackageFileName;
		UPackage* Package = CreatePackage(*PackagePath);
		SemanticClass.PlainColorMaterialInstance = Cast<UMaterialInstanceConstant>(Factory->FactoryCreateNew(
			UMaterialInstanceConstant::StaticClass(),
			Package,
			*PackageFileName,
			RF_Public | RF_Transient,
			NULL,
			GWarn));

		if (SemanticClass.PlainColorMaterialInstance == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("%s: Could not create the plain color material instance"),
				*FString(__FUNCTION__))
			check(SemanticClass.PlainColorMaterialInstance)
		}
		SemanticClass.PlainColorMaterialInstance->SetVectorParameterValueEditorOnly(
			*SemanticColorParameter,
			SemanticClass.Color);
	}

	return SemanticClass.PlainColorMaterialInstance;
}
